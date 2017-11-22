#include <filesys/inode.h>
#include <device/ata.h>
#include <device/block.h>
#include <bitmap.h>
#include <device/console.h>
#include <string.h>

//Block 12~15 valid inode number = 256
//inode size = 64byte, block size = 4096byte, 64 inode/block

char tmpblock[SSU_BLOCK_SIZE];
extern struct process *cur_process;

void init_inode_table(struct ssu_fs *fs) {
    struct bitmap *imap = fs->fs_inodemap;
    int i;
    struct inode *root;
    for (i = 0; i < NUM_INODEBLOCK; i++)
        fs_readblock(fs, SSU_INODE_BLOCK + i, ((char *) inode_tbl) + (i * SSU_BLOCK_SIZE));

    printk("init_inode_table\n");

    if (!bitmap_test(imap, INO_ROOTDIR)) {
        memset(inode_tbl, 0, sizeof(struct inode) * NUM_INODE);
        //Unvalid, Reserved
        bitmap_set(imap, 0, true);
        bitmap_set(imap, 1, true);
        inode_tbl[0].sn_ino = 0;
        inode_tbl[0].sn_size = 0;
        inode_tbl[0].sn_type = SSU_TYPE_DIR;//2
        inode_tbl[0].sn_nlink = 0;
        inode_tbl[0].sn_refcount = 0;
        inode_tbl[0].sn_fs = fs;

        //make root directory
        root = inode_create(fs, SSU_TYPE_DIR);
        memcpy(&inode_tbl[2],root, sizeof(struct inode));
        //root = inode_create(fs, SSU_TYPE_DIR);
        //cur_process->rootdir = root;

    }
    /* Add Your Code
    You should
    Set Process->cwd,rootdir
    */
    printk("cur_process : %d\n", cur_process->pid);
    cur_process->cwd = &inode_tbl[2];
    cur_process->rootdir = &inode_tbl[2];

}

struct inode *inode_create(struct ssu_fs *fs, uint16_t type) {

    struct inode *new_inode;
    struct bitmap *inodemap = fs->fs_inodemap;
    struct bitmap *bitmap = fs->fs_blkmap;
    int i;
    /* Add Your Code */

    memset(new_inode,0,sizeof(struct inode));
    for (i = 0; i < NUM_INODE; i++) {
        if (!bitmap_test(inodemap, i)) {
            new_inode->sn_ino = i;
            new_inode->sn_size = 0;
            new_inode->sn_type = type;
            new_inode->sn_nlink = 1;
            new_inode->sn_directblock[0] = 8;
            new_inode->sn_refcount = 0;
            new_inode->sn_fs = fs;
            bitmap_set(inodemap, i, true);
            bitmap_set(bitmap,i,true);
            break;
        }
    }
    view_map
    //sync_inode(fs, new_inode);
    return new_inode;
}

int inode_write(struct inode *in, uint32_t offset, char *buf, int len) {
    printk("inode_write\n");

    int result = 0;
    struct ssu_fs *fs = in->sn_fs;
    /* Add Your Code */
    fs_readblock(fs, SSU_DATA_BLOCK,tmpblock);

    //tmpblock에 추가
    memcpy(&tmpblock[offset*INODE_PER_BLOCK], buf,len);

    fs_writeblock(fs, SSU_DATA_BLOCK, tmpblock);

    return result;
}

int inode_read(struct inode *in, uint32_t offset, char *buf, int len) {
    int result = 0;
    struct ssu_fs *fs = in->sn_fs;

    /* Add Your Code */





    return result;
}

static int sync_inode(struct ssu_fs *fs, struct inode *inode) {
    int result = 0;
    int offset = inode->sn_ino / INODE_PER_BLOCK; //offset to current inode index from SSU_INODE_BLOCK

    result = fs_writeblock(fs, SSU_INODE_BLOCK + offset, (char *) (inode_tbl + offset * INODE_PER_BLOCK));
    return result;
}


int make_dir(struct inode *cwd, char *name) {
    /* Add Your Code */
    printk("make_dir func income\n");
    struct ssu_fs *fs = cwd->sn_fs;
    //struct inode *new_inode = inode_create(fs, SSU_TYPE_DIR);
//    struct direntry new_dir;
//    new_dir.de_ino = new_inode->sn_ino;
//    memcpy(new_dir.de_name, name, FILENAME_LEN);
//
//    inode_write(new_inode,new_dir.de_ino,(char *)&new_dir, sizeof(struct direntry));
    return 0;
}

static int num_direntry(struct inode *in) {
    if (in->sn_size % sizeof(struct direntry) != 0 || in->sn_type != SSU_TYPE_DIR)
        return -1;

    return in->sn_size / sizeof(struct direntry);
}


void list_segment(struct inode *cwd) {
    /* Add Your Code */


}

int change_dir(struct inode *cwd, char *path) {
    /* Add Your Code */

    return -1;
}

int get_curde(struct inode *cwd, struct direntry *de) {
    struct inode *pwd;
    int i, ndir;

    //get parent dir
    inode_read(cwd, 0, (char *) de, sizeof(struct direntry));
    pwd = &inode_tbl[de->de_ino];
    ndir = num_direntry(pwd);
    for (i = 2; i < ndir; i++) {
        inode_read(pwd, i * sizeof(struct direntry), (char *) de, sizeof(struct direntry));
        if (de->de_ino == cwd->sn_ino)
            return 0;
    }
    return -1;
}