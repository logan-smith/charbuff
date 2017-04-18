/*
Modified version of code provided by derekmolloy.ie
*/
#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/mutex.h>          // For Mutex functions
#include <asm/uaccess.h>          // Required for the copy to user function
//Does this get renamed to charbuffin?
#define  DEVICE_NAME "charbuffin"    ///< The device will appear at /dev/charbuff using this value
#define  CLASS_NAME  "charbin"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("COP 4600-17 Group 12");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users


static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[1024] = {0};           ///< Memory for the string that is passed from userspace
static char   charBuffer[1025] = {0};
// I think both modules can use this???
// extern char charBuffer[1025] = {0};
EXPORT_SYMBOL(charBuffer);

int charBuffLen = 0;
EXPORT_SYMBOL(charBuffLen);

static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  charbuffClass  = NULL; ///< The device-driver class struct pointer
static struct device* charbuffDevice = NULL; ///< The device-driver device struct pointer

// Declare a new mutex
static DEFINE_MUTEX(charbuff_mutex);

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
// static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   // .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init charbuff_init(void){
   printk(KERN_INFO "charbuff: Initializing the charbuff LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "charbuff failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "charbuff: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   charbuffClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(charbuffClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(charbuffClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "charbuff: device class registered correctly\n");

   // Register the device driver
   charbuffDevice = device_create(charbuffClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(charbuffDevice)){               // Clean up if there is an error
      class_destroy(charbuffClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(charbuffDevice);
   }
   printk(KERN_INFO "charbuff: device class created correctly\n"); // Made it! device was initialized
   
   // Initialize the mutex
   mutex_init(&charbuff_mutex);

   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit charbuff_exit(void){
   // Destroy the mutex we created
   mutex_destroy(&charbuff_mutex);

   device_destroy(charbuffClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(charbuffClass);                          // unregister the device class
   class_destroy(charbuffClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "charbuff: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){

// Try to aquire the mutex
// Returns 0 if it's already held
   if(!mutex_trylock(&charbuff_mutex))
   {
      printk(KERN_ALERT "charbuff: Device in use by another process");
      return -EBUSY;
   }

   numberOpens++;
   printk(KERN_INFO "charbuff: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
// static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
//    int error_count = 0;
//    // copy_to_user has the format ( * to, *from, size) and returns 0 on success
//    //error_count = copy_to_user(buffer, message, size_of_message);
// 	int length = (int) len;
// 	char *temp;
// 	//requested length is less than or equal to total in buffer
// 	if(length <= charBuffLen)
// 	{
// 		error_count = copy_to_user(buffer, charBuffer, length);
// 		//strcpy(temp, charBuffer);
// 		//strncpy(temp, charBuffer + length, charBuffLen - length);
// 		temp = charBuffer + length;
// 		strcpy(charBuffer,temp);
// 		charBuffLen -= length;
// 	}
// 	//if requested length is greater than total buffer
// 	else
// 	{
// 		error_count = copy_to_user(buffer, charBuffer, charBuffLen);
// 		//strcpy(charBuffer, '');
// 		charBuffLen = 0;
// 	}
	
//    if (error_count==0){            // if true then have success
//       printk(KERN_INFO "charbuff: Sent %d characters to the user\n", size_of_message);
//       return (size_of_message=0);  // clear the position to the start and return 0
//    }
//    else {
//       printk(KERN_INFO "charbuff: Failed to send %d characters to the user\n", error_count);
//       return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
//    }
// }

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   sprintf(message, "%s", buffer);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   // if adding new string exceeds buffer size, cut it
	char temp[1024];   
	if(size_of_message + charBuffLen > 1024)
   {
   		int lenToWrite = 1024-charBuffLen;
		strncpy(temp, message, lenToWrite);
   		if(charBuffLen == 0)
		{
			charBuffer[0] = '\0';
			strcpy(charBuffer, temp);
			//charBuffLen += size_of_message;
		}
		
		else
		{
			strcat(charBuffer, temp);
			strcat(charBuffer, '\0');
		}
		charBuffLen += lenToWrite;
   }
   //if new string fits, add it as is
   else
   {
   		if(charBuffLen == 0)
		{
			charBuffer[0] = '\0';
			strcpy(charBuffer, message);
			charBuffLen += size_of_message;
		}
		else if(charBuffLen > 0)
		{
			strcat(charBuffer, message);
			charBuffLen += size_of_message;
		}
   }
   printk(KERN_INFO "charbuff: Received %zu characters from the user\n", len);
   return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){

   // Release the mutex
   mutex_unlock(&charbuff_mutex);

   printk(KERN_INFO "charbuff: Device successfully closed\n");
   return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(charbuff_init);
module_exit(charbuff_exit);
