#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

int init_module(void);
void cleanup_module(void);
// Is this how you prototype these?
struct node *create(int data);
struct node *pop();

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file*, char *, size_t, loff_t *);
static ssize_t device_write(struct file*, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "charbuff" // The name that will be in /proc/devices
#define BUF_LEN 80 // Max length, should be 1KB?
// Linked list for buffer queue struct????

struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
	.open = device_open,
	.release = device_release
};

struct node
{
	int data;
	node *next;
};

node *front = NULL;
node *rear = NULL;

node *create(int data)
{
	temp = (struct node *)malloc(sizeof(struct node));
	temp->data = data;
	temp->next = null;
	return temp; 
}

void insert(int item)
{
	struct node *newnode = create(item);

	if (front == NULL)
	{
		front = rear = newnode;
	}
	else
	{
		rear->next = newnode;
		rear = newnode;
	}
}

node *pop()
{
	struct node *keep = front;
	struct node *temp = front;

	// Queue is empty so just leave
	if (front == NULL)
	{
		return;
	}
	// Delete the top
	else
	{
		front = front->next;
		if(front == NULL)
			rear = NULL;
		free(temp);
		return(keep);
	}
}


static int Major;
// So device isn't opened more than once.
static int Device_Open = 0; 
// msg device will provide.
static char msg[BUF_LEN];

int init_module(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0 )
	{
		printk(KERN_ALERT "Registering device failed\n");
		return Major;
	}

	printk(KERN_INFO "I was assigned major number %d\n", Major);
	printk(KERN_INFO "Installing module named %s.\n", DEVICE_NAME);
//major is the major number we want
// ^ Set to 0 so it gives a random one
//temp is the name that will show up in /proc/devices
//
	return 0;
}

void cleanup_module(void) 
{
	printk(KERN_INFO "Removing module.\n");
	int ret = unregister_chrdev(Major, DEVICE_NAME);
	if (ret < 0)
		printk(KERN_ALERT "Error in ungregistering\n");
}

//Sample code 
//Called when a process tries to open the device file
// like "cat /dev/mycharfile"
static int device_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device Opened\n");

	// This is all sample code stuff
	// I don't think we need to keep track of any of this stuff.

        static int counter = 0;
        if (Device_Open)
                return −EBUSY;
        Device_Open++;
        sprintf(msg, "I already told you %d times Hello world!\n", counter++);
        msg_Ptr = msg;
        try_module_get(THIS_MODULE);
        return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "Device Closed\n");

	// Again I dont know if we need this stuff


    Device_Open−−;          /* We're now ready for our next caller */
        /* 
         * Decrement the usage count, or else once you opened the file, you'll
         * never get get rid of the module. 
         */
    module_put(THIS_MODULE);
    return 0;
}

// Called when a process, which has already opened the dev file, 
// attempts to read from it
//
// I don't really understand the buffer or how the user reads, 
// but its something to go off of.
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
	printk(KERN_INFO "Device Read\n");

	// Pop the queue in here

	struct node *temp = pop(); 

	

	// Should we use put_user or copy_to_user idk idk idk
	// 
	// How does this work I have no idea.
	put_user(temp->data, buffer++);

	// What should be returned?
    return;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	// Add to queue in here.

	printk(KERN_INFO "Device Written To\n");

	insert(buff);


	// idk what this does.
	return -EINVAL;
}