#include <linux/module.h> //needed by all modules
#include <linux/kernel.h> //needed for KERN_INFO
#include <linux/sched.h> //defined struct task_struct
#include <linux/proc_fs.h> //create proc file
#include <linux/seq_file.h> //use seq_file
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#define next_task(p)    list_entry(rcu_dereference((p)->tasks.next), struct task_struct, tasks)
#define FILENAME "project1_2" //defile proc file name

// task_struct we need 
extern struct task_struct tasks;

// functions we need
int print_proc(struct seq_file *, void *); 
int open_proc(struct inode *, struct file *);

// structure for creating proc file
// open proc file(using seq_file API)
int open_proc(struct inode *inode, struct file *file)
{
    single_open(file, print_proc, NULL);//seq API and print_proc
    return 0;
}
static const struct file_operations op = 
{
        .owner = THIS_MODULE,
        .open = open_proc,// the open function we stated 
        .read = seq_read,
        .release = single_release
};




// what we want proc to do
int print_proc(struct seq_file *file,void *v)
{
    // print 
    seq_printf(file,"[m] Print all processes information:\n");
    
    // init variables
    int run_num = 0; //the number of process whose status is running
    int num_interruptible = 0; //the number of process whose status is interruptible
    int num_uninterruptible = 0; //the ... status is uninterruptible
    int num_zombie = 0; //the process exited with status zombie
    int num_stopped = 0; //the ... status is stopped
    int num_traced = 0; //the ... status is traced
    int num_dead = 0; //the process has deaded;
    int num_unknown = 0; //the process whose status is unknown
    int num_total = 0; //the total number of process
    struct task_struct *p; //pointer to task_struct
   
    // tasks is the head of the tasks
    for(p=&tasks; p!=&tasks; p=next_task(p))
    { 
	// print the Name and Pid and State and ParentName
        seq_printf(file,"[m] Name:%s Pid:%d State:%ld Parent:%s\n",p->comm
                                                                   ,p->pid
                                                                   ,p->state
                                                         ,p->real_parent->comm);
        //num ++ 
        num_total++; 
        
        // if the process is stopped 
        if(p->exit_state != 0)
        { 
            switch(p->exit_state)
            {
                case EXIT_ZOMBIE://if the exit state is zombie
                    num_zombie++;//variable plus one
                    break; //break switch 
                case EXIT_DEAD://if the exit state is dead
                    num_dead++;//variable plus one
                    break;//break switch
                default: //other case
                    break;//break switch
            }
        }
        // if not
        else
        {
            switch(p->state)
            {
                case TASK_RUNNING://if the state is running
                    run_num++;//variable plus one
                    break;//break switch
                case TASK_INTERRUPTIBLE://state is interruptible
                    num_interruptible++;//variable plus one
                    break;//break switch
                case TASK_UNINTERRUPTIBLE://state is uninterruptible
                    num_uninterruptible++;//var + 1
                    break;//break switch
                case TASK_STOPPED://state is stopped
                    num_stopped++;//var +1
                    break;//break switch
                case TASK_TRACED://state is traced
                    num_traced++;//var +1
                    break;//break switch
                default://other case
                    num_unknown++;
                    break;
            }
        }
    }
    //below instruction is to print the statistics result in above code
     printk(KERN_INFO "[m] total tasks: %10d\n",num_total);
     printk(KERN_INFO "[m] TASK_RUNNING: %10d\n",run_num);
     printk(KERN_INFO "[m] TASK_INTERRUPTIBLE: %10d\n",num_interruptible);
     printk(KERN_INFO "[m] TASK_UNINTERRUPTIBLE: %10d\n",num_uninterruptible);
     printk(KERN_INFO "[m] TASK_TRACED: %10d\n",num_stopped);
     printk(KERN_INFO "[m] TASK_TRACED: %10d\n",num_stopped);
     printk(KERN_INFO "[m] EXIT_ZOMBIE: %10d\n",num_zombie);
     printk(KERN_INFO "[m] EXIT_DEAD: %10d\n",num_dead);
     printk(KERN_INFO "[m] UNKNOWN: %10d\n",num_unknown);
    seq_printf(file,"[m] total tasks: %10d\n",num_total);
    seq_printf(file,"[m] TASK_RUNNING: %10d\n",run_num);
    seq_printf(file,"[m] TASK_INTERRUPTIBLE: %10d\n",num_interruptible);
    seq_printf(file,"[m] TASK_UNINTERRUPTIBLE: %10d\n",num_uninterruptible);
    seq_printf(file,"[m] TASK_TRACED: %10d\n",num_stopped);
    seq_printf(file,"[m] TASK_TRACED: %10d\n",num_stopped);
    seq_printf(file,"[m] EXIT_ZOMBIE: %10d\n",num_zombie);
    seq_printf(file,"[m] EXIT_DEAD: %10d\n",num_dead);
    seq_printf(file,"[m] UNKNOWN: %10d\n",num_unknown);

    return 0;
} 
// create proc file
int init_proc(void)
{
    struct proc_dir_entry * myfile;
//0444
    myfile = proc_create(FILENAME, 0, NULL, &op);//create proc file
    if(myfile == NULL) //deal with error
        return -ENOMEM;
    printk(KERN_INFO "proc file:%s inited\n", FILENAME);//print debug message
    return 0;
}

// start kernel module
int init_module(void){
    int err = init_proc();// init proc
    print_proc(struct seq_file *,NULL);// write into seq_file 
    printk(KERN_INFO "project1_2 launched\n");// debugging
    return err;
}


// remove proc file
void remove_proc(void)
{
    remove_proc_entry(FILENAME, NULL);//remove proc file
    printk(KERN_INFO "proc file:%s removed\n", FILENAME);//print debug message
}

// end kernel module
void cleanup_module(void){ //clean up the resources when module finished
    remove_proc();//remove proc file
    printk(KERN_INFO "project1_2 ended\n");//print finish message
}

MODULE_LICENSE("GPL");//show that this code follow GUN General Public License
