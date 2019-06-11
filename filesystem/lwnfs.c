#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */
#include<linux/sched.h>
/*
 * Boilerplate stuff.
 */
MODULE_LICENSE("GPL");
#define LFS_MAGIC 0x19980122

static struct inode *lfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_mode = mode;
		ret->i_ino = get_next_ino();
		i_uid_write(ret, 0);
		i_gid_write(ret, 0);
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;

	}
	return ret;
}

static int lfs_open(struct inode *inode, struct file *filp)
{
	filp->private_data = inode->i_private;
	return 0;
}

#define TMPSIZE 2000

static ssize_t lfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{


	char str_stat[40];
	int int_stat;
	int len;
	char tmp_str[2000];
	unsigned long mem_pg;
	unsigned long mem_usage;
	struct task_struct *cur_task = (struct task_struct *) filp->private_data;
	len = 0;
	int_stat = cur_task->state;
	if(cur_task->mm != NULL){
	mem_pg = cur_task->mm->total_vm;
	mem_usage = mem_pg << PAGE_SHIFT; 
	}
	switch(int_stat){
		case 0:
			snprintf(str_stat, 40,  "TASK_RUNNING");
			break;
		case 1:
			snprintf(str_stat, 40, "TASK_INTERRUPTIBLE");
			break;
		case 2:
			snprintf(str_stat, 40, "TASK_UNINTERRUPTIBLE");
			break;
		case 4:
			snprintf(str_stat, 40, "__TASK_STOPPED");
			break;
		case 8:
			snprintf(str_stat, 40, "__TASK_TRACED");
			break;
		case 16:
			snprintf(str_stat, 40, "EXIT_DEAD");
			break;
		case 32:
			snprintf(str_stat, 40, "EXIT_ZOMBIE");
			break;
		case 64:
			snprintf(str_stat, 40, "TASK_DEAD");
			break;
		case 128:
			snprintf(str_stat, 40, "TASK_WAKEKILL");
			break;
		case 256:
			snprintf(str_stat, 40, "TASK_WAKING");
			break;
		case 512:
			snprintf(str_stat, 40, "TASK_PARKED");
			break;
		case 1024:
			snprintf(str_stat, 40, "TASK_STATE_MAX");
			break;
		default:
			snprintf(str_stat, 40, "Undefined");
			break;
	}


	if(cur_task->mm != NULL){
	len = snprintf(tmp_str, TMPSIZE, "\n Process name: %s\n Process pid: %d\n State: %s\n Kernel thread: %s\n"
			" CPU: %d\n Start time: %llu\n Size of virtual stack : %lu pages\n Memory usage: %ld Bytes\n Stack Pointer of the kernel: %lx\n Stack Pointer of the user: %lx\n",
			cur_task->comm, cur_task->pid,
			str_stat, (cur_task->mm)?"NO":"YES", 
			task_thread_info(cur_task)->cpu,
			cur_task->start_time,
			cur_task->mm->stack_vm, mem_usage,cur_task->thread.sp, cur_task->thread.usersp);//(total_vm << PAGE_SHIFT));
	}else{

	len = snprintf(tmp_str, TMPSIZE, "\n Process name: %s\n Process pid: %d\n State: %s\n Kernel thread: %s\n"
			" CPU: %d\n Start time: %llu\n Size of virtual stack : %s \n Memory usage: %s \n Stack Pointer of the kernel: %lx\n Stack Pointer of the user: %lx\n",
			cur_task->comm, cur_task->pid,
			str_stat,"YES", 
			task_thread_info(cur_task)->cpu,
			cur_task->start_time,
			"Not Applicable because mm field is Null", "Not Applicable because mm field is Null",cur_task->thread.sp, cur_task->thread.usersp);//(total_vm << PAGE_SHIFT));
	}

	if (count > len - *offset)
		count = len - *offset;
	if (copy_to_user(buf, tmp_str + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static ssize_t lfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	struct task_struct *cur_task = (struct task_struct *) filp->private_data;
	char tmp[TMPSIZE];
	int sig_num;
	if (*offset != 0)
		return -EINVAL;
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	sig_num = simple_strtol(tmp, NULL, 10);	
	send_sig(sig_num, cur_task, 1);
	return count;
}


static struct file_operations lfs_file_ops = {
	.open	= lfs_open,
	.read 	= lfs_read_file,
	.write  = lfs_write_file,
};


static struct dentry *lfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name, struct task_struct *tsk)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;
	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = lfs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &lfs_file_ops;
	inode->i_private = tsk;
	d_add(dentry, inode);
	return dentry;
out_dput:
	dput(dentry);
out:
	return 0;
}


static struct dentry *lfs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(parent, &qname);
	if (! dentry)
		goto out;

	inode = lfs_make_inode(sb, S_IFDIR | 0755);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	inode_init_owner(inode, NULL, S_IFDIR);
	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}


static struct dentry* lfs_create_container(struct super_block *sb, struct dentry* root_dir, struct task_struct* t_struct)
{

 	struct dentry *subdir;
	char tmp_str [30], tmp_str2 [40];
	int pid = t_struct->pid;
   	snprintf(tmp_str, 30, "%d", pid);
   	snprintf(tmp_str2, 30, "%d.status", pid);
	subdir = lfs_create_dir(sb, root_dir, tmp_str);
	if(subdir){
	lfs_create_file(sb, subdir, tmp_str2, t_struct);
	lfs_create_file(sb, subdir, "signal", t_struct);
	}
return subdir;

}


static void rec_iter(struct super_block *sb, struct dentry* root_dir, struct task_struct* cur_task)
{   
	struct task_struct *child;
	struct list_head *list;
 	struct dentry *subdir;
	subdir = lfs_create_container(sb, root_dir, cur_task);
	list_for_each(list, &cur_task->children) {
		child = list_entry(list, struct task_struct, sibling);
		rec_iter(sb, subdir, child);
	}
}

static struct super_operations lfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};


static int lfs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &lfs_s_ops;
	root = lfs_make_inode (sb, S_IFDIR | 0755);
	if (! root)
		goto out;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;
	inode_init_owner(root, NULL, S_IFDIR);
	root_dentry = d_make_root(root);
	if (! root_dentry)
		goto out_iput;
	sb->s_root = root_dentry;
	rec_iter(sb, root_dentry,&init_task);
	return 0;
	
  out_iput:
	iput(root);
  out:
	return -ENOMEM;
}


static struct dentry *lfs_get_super(struct file_system_type *fst,
		int flags, const char *devname, void *data)
{
	return mount_nodev(fst, flags, data, lfs_fill_super);
}

static struct file_system_type lfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "lwnfs",
	.mount		= lfs_get_super,
	.kill_sb	= kill_litter_super,
};

static int __init lfs_init(void)
{
	return register_filesystem(&lfs_type);
}

static void __exit lfs_exit(void)
{
	unregister_filesystem(&lfs_type);
}

module_init(lfs_init);
module_exit(lfs_exit);
