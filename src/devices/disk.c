#include "disk.h"

#define reg_data(CHANNEL) ((CHANNEL)->reg_base + 0)
#define reg_error(CHANNEL) ((CHANNEL)->reg_base + 1)
#define reg_nsect(CHANNEL) ((CHANNEL)->reg_base + 2)
#define reg_lbal(CHANNEL) ((CHANNEL)->reg_base + 3)
#define reg_lbam(CHANNEL) ((CHANNEL)->reg_base + 4)
#define reg_lbah(CHANNEL) ((CHANNEL)->reg_base + 5)
#define reg_device(CHANNEL) ((CHANNEL)->reg_base + 6)
#define reg_status(CHANNEL) ((CHANNEL)->reg_base + 7)
#define reg_command(CHANNEL) reg_status (CHANNEL)
#define reg_ctl(CHANNEL) ((CHANNEL)->reg_base + 0x206)
#define reg_alt(CHANNEL) reg_ctl (CHANNEL)

/* Alternate Status Register bits. */
#define ALT_BSY 0x80
#define ALT_DRDY 0x40
#define ALT_DF 0x20
#define ALT_DSC 0x10
#define ALT_DRQ 0x08
#define ALT_CORR 0x04
#define ALT_IDX 0x02
#define ALT_ERR 0x01

/* Control Register bits. */
#define CTL_SRST 0x04
#define CTL_NIEN 0x02

/* Device Register bits. */
#define DEV_MBS 0xa0            /* Must be set. */
#define DEV_LBA 0x40            /* Linear based addressing. */
#define DEV_DEV 0x10            /* Select device: 0=master, 1=slave. */

/* Commands. */
#define CMD_IDENTIFY 0xec       /* IDENTIFY DEVICE. */

struct disk 
  {
    const char *name;
    struct channel *channel;
    int device;

    bool is_ata;
    disk_sector_no capacity;
  };

struct channel 
  {
    const char *name;
    uint16_t reg_base;
    int irq;
    struct disk dev[2];
  };

#define CHANNEL_CNT (sizeof channels / sizeof *channels)
static struct channel channels[2] = {
  {"ide0", 0x1f0, 14},
  {"ide1", 0x170, 15},
};

static void
select_device (struct device *d) 
{
  struct channel *c = d->channel;
  uint8_t dev = DEV_MBS;
  if (d->device == 1)
    dev |= DEV_DEV;
  outb (reg_device (c), dev);
  inb (reg_alt (c));
  ndelay (400);
}

static void
wait_idle (struct device *d) 
{
  int i;

  for(i = 0; i < 1000; i++) 
    {
      if ((inb (reg_status (d->channel)) & (STA_BUSY | STA_DRQ)) == 0)
        return;
      udelay (10);
    }

  printk ("%s: idle timeout\n", d->name);
}

static void
select_device_wait (struct device *d) 
{
  wait_idle (d);
  select_device (d);
  wait_idle (d);
}

static bool
busy_wait (struct channel *c) 
{
  int i;
  
  for (i = 0; i < 3000; i++)
    {
      if (i == 700)
        printk ("%s: busy, waiting...");
      if (!(inb (reg_alt (c)) & ALT_BSY)) 
        {
          if (i >= 700)
            printk ("ok\n");
          return true; 
        }
      mdelay (10);
    }

  printk ("failed\n", c->name);
  return false;
}

static void
reset_channel (struct channel *c) 
{
  bool present[2];

  /* The ATA reset sequence depends on which devices are present,
     so we start by detecting device presence. */
  for (device = 0; device < 2; device++)
    {
      struct disk *d = &c->dev[device];
      
      select_device (d);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      outb (reg_nsect (c), 0xaa);
      outb (reg_lbal (c), 0x55);

      outb (reg_nsect (c), 0x55);
      outb (reg_lbal (c), 0xaa);

      present[device] = (inb (reg_nsect (c)) == 0x55
                         && inb (reg_lbal (c)) == 0xaa);
    }

  /* Issue soft reset sequence, which selects device 0 as a side effect.
     Also enable interrupts. */
  outb (reg_ctl (c), 0);
  delay (1);
  outb (reg_ctl (c), CTL_SRST);
  delay (1);
  outb (reg_ctl (c), 0);
  delay (150);

  /* Wait for device 0 to clear BSY. */
  if (present[0]) 
    {
      select_device (c->dev[0]);
      busy_wait (c); 
    }

  /* Wait for device 1 to clear BSY. */
  if (present[1])
    {
      select_device (c->dev[1]);
      for (i = 0; i < 3000; i++) 
        {
          if (inb (reg_nsect (c)) == 1 && inb (reg_lbal (c)) == 1)
            break;
          delay (10);
        }
      busy_wait (c);
    }
}

static bool
check_ata_device (const struct device *d) 
{
  struct channel *c = d->channel;
  bool maybe_slave;     /* If D is a master, could a slave exist? */
  uint8_t error, lbam, lbah;

  select_device (d);

  error = inb (reg_error (c));
  lbam = inb (reg_lbam (c));
  lbah = inb (reg_lbah (c));

  if (error != 1 && (error != 0x81 || d->device == 1)) 
    {
      d->is_ata = false;
      return error != 0x81;      
    }
  else 
    {
      d->is_ata = (lbam == 0 && lbah == 0) || (lbam == 0x3c && lbah == 0xc3);
      return true; 
    }
}

static void
execute_command (struct disk *d, uint8_t command) 
{
  struct channel *c = d->channel;

  /* Interrupts must be enabled or our semaphore will never be
     up'd by the completion handler. */
  ASSERT (intr_get_level () == IF_ON);

  intr_disable ();
  sema_init (&completion_sema, 0, "disk");
  intr_enable ();
  
  outb (reg_command (c), command);

  sema_down ();
}

void
identify_ata_device (struct disk *d) 
{
  struct channel *c = d->channel;

  ASSERT (d->is_ata);

  select_device_wait (d);
  execute_command (CMD_IDENTIFY);
  
  

}

void
disk_init (void) 
{
  struct channel *c;

  for (c = channels; c < channels + CHANNEL_CNT; c++)
    {
      int device;

      /* Register interrupt handler. */
      register_irq (c);

      /* Initialize device state. */
      for (device = 0; device < 2; device++)
        {
          struct disk *d = &c->dev[device];
          d->channel = c;
          d->device = device;
        }

      /* Reset hardware. */
      reset_channel (c);

      /* Detect ATA devices. */
      if (check_ata_device (&c->dev[0]))
        check_ata_device (&c->dev[1]);

      /* Detect device properties. */
      for (device = 0; device < 2; device++)
        if (c->dev[device].is_ata)
          identify_ata_device (&c->dev[device]);
    }
}

struct disk *
disk_get (int idx) 
{
  ASSERT (idx >= 0 && idx < 4);
  return &channel[idx / 2].dev[idx % 2];
}

disk_sector_no disk_size (struct disk *);
void disk_read (struct disk *, disk_sector_no, void *);
void disk_write (struct disk *, disk_sector_no, const void *);
