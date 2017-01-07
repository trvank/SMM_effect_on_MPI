/*
  Experimental driver for generating SMIs. 
  For use in CS533 OS class.
*/

#include <linux/proc_fs.h>
#include <linux/pci.h>

// Controls scheduling rate of SMIs. Can be adjusted. 
#define DRIVER_RATE 950

// User commands
#define TEST_LATENCY 1
#define TEST_LATENCY_LONGER 2
#define DRIVER_WORKQ_START_SHORT 3
#define DRIVER_WORKQ_START_LONG 4
#define DRIVER_WORKQ_STOP 5
#define CHECK_SMI_COUNT 6
#define GET_TSC 7

#define OFF 0
#define ON 1
#define SHORT 0
#define LONG 1

// Type of scheduled SMI to generate (e.g. short or long)
int g_smi_duration=SHORT;

struct delayed_work *work;

int read_proc_file(char *page, char** start, off_t off, int count, int* eof, void* data);
int write_proc_file(struct file* file, const char* buffer, unsigned long count, void *data);
int generate_smi(unsigned short port, unsigned char val);
static __inline__ unsigned long long readtsc(void);

int workq_enabled=OFF;

// This function gets called on a regular basis when scheduled
static void scheduled_function( struct work_struct *work)
{
  // This function calls generate_smi which will actually generate the SMI
  if (g_smi_duration == SHORT)
    generate_smi(0xb2,0x0); // short SMI
  else if (g_smi_duration == LONG)
    generate_smi(0xb2,0x34); // long SMI
  else {
    printk("\nWarning. Can't determine g_smi_duration.");
    return;
  }

  schedule_delayed_work((struct delayed_work*)work, DRIVER_RATE); // Schedule the next SMI in the future
  return;
}

// Return the CPU's timestamp counter (TSC)
static __inline__ unsigned long long readtsc(void)
{
  unsigned a,d;
  asm volatile("rdtsc" : "=a"(a), "=d" (d));
  return ((unsigned long long)a | ((unsigned long long)d) << 32);
}

// Kernel module initialization
static int __init smidriver_init(void)
{
  struct proc_dir_entry *smidriver;
  smidriver = create_proc_entry("smidriver", 0666, NULL);

  if (smidriver !=NULL) {
    smidriver->read_proc=read_proc_file;
    smidriver->write_proc=write_proc_file;
  }

  return 0;
}

// Kernel module cleanup upon unloading
static void __exit smidriver_exit()
{
  remove_proc_entry("smidriver", NULL);
  if (workq_enabled) {
    cancel_delayed_work_sync(work);
    kfree( (void *)work );
  }
  
  printk("\nExiting");
}
module_init(smidriver_init);  
module_exit(smidriver_exit);

// Code added here will be called when you do 'cat /proc/smidriver' from the Linux command prompt   
int read_proc_file(char *page, char** start, off_t off, int count, int* eof, void* data)
{
  

  return 0;
}

// This function receives linux command shell commands, e.g. "echo 1 > /proc/smidriver" to do TEST_LATENCY. 
int write_proc_file(struct file* file, const char* buffer, unsigned long count, void *data)
{
  unsigned long long start=0, end=0;
  char **endp=NULL;
  int option=0;
  int i=0;
  unsigned int start_smi_count=0,lo=0;     
  
  // Change this to generate more SMIs back to back for TEST_LATENCY and TEST_LATENCY_LONGER commands.
  int smi_count=10;
  
  // Interpret the user's option
  option = simple_strtoul(buffer, endp, 10);

  switch (option) {
    
    // Do several short SMIs back to back and measure how long it took
  case TEST_LATENCY:
    start = readtsc();

    for (i=0; i < smi_count; i++) {
      generate_smi(0xb2, 0);
    }

    end = readtsc();
    printk("\nTesting SMI Latency: %llu CPU clocks.", (end - start)/smi_count);
    break;
    // Do several longer SMIs and measure how long it took
  case TEST_LATENCY_LONGER:
    start = readtsc();
    for (i=0; i < smi_count; i++) {
      generate_smi(0xb2, 0x34);
    }
    end = readtsc();
    printk("\nTesting SMI Latency: %llu CPU clocks.", (end - start)/smi_count);
    break;
    // Report back how many SMIs have been processed so far
  case CHECK_SMI_COUNT:
    rdmsr(0x34,start_smi_count,lo);
    printk("\nSMI Count %u", start_smi_count);
    break;
    // Turn on regularly occuring SMIs. This will trigger the function "scheduled_function" to execute at a later time.
  case DRIVER_WORKQ_START_SHORT: // fall through to next case
  case DRIVER_WORKQ_START_LONG:

    if (option == DRIVER_WORKQ_START_SHORT)
      g_smi_duration = SHORT;
    else if (option == DRIVER_WORKQ_START_LONG)
      g_smi_duration = LONG;
    else {
      printk("\nWarning. Unable to determine g_smi_duration.");
      break;
    }

    if (workq_enabled)
      break;
    work = (struct delayed_work *)kmalloc(sizeof(struct delayed_work), GFP_KERNEL);

    if (work) {
      workq_enabled=ON;
      INIT_DELAYED_WORK( (struct delayed_work *)work, scheduled_function );
      schedule_delayed_work((struct delayed_work*)work, DRIVER_RATE);
    }
    break;
    // Turn off regularly occurring SMIs
  case DRIVER_WORKQ_STOP:
    if (workq_enabled) {
      cancel_delayed_work_sync(work);
      workq_enabled=OFF;
      kfree( (void *)work );
    }
    break;
    // Report back the CPU's Timestamp counter (TSC). The TSC is just a counter that increments by one by every CPU clock tick. (E.g. 2.26 billion times a second for a 2.26 GHz CPU.)
  case GET_TSC:
    start = readtsc();
    printk("\nTSC %llu", start);
    break;
  default:
    printk("\nUnexpected input");
    break;
  }
  
  return count;
}

// Generate an SMI. (E.g. writing a value to port 0xb2 generates some SMI.)
int generate_smi(unsigned short port, unsigned char val)
{
  asm volatile("outb %0, %1"
	       :
	       : "a"(val), "Nd"(port)
	       );
  return 0;
}



