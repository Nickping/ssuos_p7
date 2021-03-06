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
    struct bitmap *bmap = fs->fs_blkmap;
    int i;
    struct inode *rootInode;
    for (i = 0; i < NUM_INODEBLOCK; i++) {
        fs_readblock(fs, SSU_INODE_BLOCK + i, ((char *) inode_tbl) + (i * SSU_BLOCK_SIZE));

    }

    for (i = 0; i < 8; i++)
        bitmap_set(bmap, i, true);

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
        rootInode = inode_create(fs, SSU_TYPE_DIR);
        //memcpy(&inode_tbl[2], rootInode, sizeof(struct inode));
        //root = inode_create(fs, SSU_TYPE_DIR);
        //cur_process->rootdir = root;
        memset(tmpblock, 0, sizeof(tmpblock));
        struct direntry *dataBlock = (struct direntry *) tmpblock;

        struct direntry cur_dir__ = {2, "."};
        struct direntry parent_dir = {2, ".."};

        memcpy(&dataBlock[0], &cur_dir__, sizeof(struct direntry));
        memcpy(&dataBlock[1], &parent_dir, sizeof(struct direntry));


        fs_writeblock(fs, 8, (const char *) dataBlock);//data 블록에 쓰기
    }
    /* Add Your Code
    You should
    Set Process->cwd,rootdir
    */
//    memset(tmpblock, 0, sizeof(tmpblock));
//    struct direntry *dataBlock = (struct direntry *) tmpblock;
//
//    struct direntry cur_dir__ = {2, "."};
//    struct direntry parent_dir = {2, ".."};
//
//    memcpy(&dataBlock[0], &cur_dir__, sizeof(struct direntry));
//    memcpy(&dataBlock[1], &parent_dir, sizeof(struct direntry));
//
//
//    fs_writeblock(fs, 8, (const char *) dataBlock);//data 블록에 쓰기

    cur_process->cwd = &inode_tbl[2];
    cur_process->rootdir = &inode_tbl[2];

}

struct inode *inode_create(struct ssu_fs *fs, uint16_t type) {

    struct inode *new_inode;
    struct bitmap *inodemap = fs->fs_inodemap;
    struct bitmap *bitmap = fs->fs_blkmap;
    int i;
    /* Add Your Code */

    memset(new_inode, 0, sizeof(struct inode));
    for (i = 0; i < NUM_INODE; i++) {
        if (!bitmap_test(inodemap, i)) {
            new_inode->sn_ino = i;
            new_inode->sn_size = 128;
            new_inode->sn_type = type;
            new_inode->sn_nlink = 1;
            new_inode->sn_directblock[0] = i + 6;
            new_inode->sn_refcount = 0;
            new_inode->sn_fs = fs;
            memcpy(&inode_tbl[i], new_inode, sizeof(struct inode));

            bitmap_set(inodemap, i, true);
            bitmap_set(bitmap, i + 6, true);
            fs_writeblock(fs,SSU_INODE_BLOCK,(char *)inodemap);
            fs_writeblock(fs, SSU_BITMAP_BLOCK, (char *) bitmap);
            break;
        }
    }

//    view_bitmap(inodemap);
//    view_bitmap(bitmap);
    return new_inode;
}

int inode_write(struct inode *in, uint32_t offset, char *buf, int len) {
//buf 를 len만큼 in으로 찾은 data_block 에다가 offset 부터 len만큼 쓴다 len == 64
    int result = 0;
    struct ssu_fs *fs = in->sn_fs;
    struct bitmap *inodemap = fs->fs_inodemap;

    fs_readblock(fs, in->sn_directblock[0], tmpblock);
    struct direntry *data = (struct direntry *) tmpblock;
    memcpy(&data[offset], (struct direntry *) buf, len);
    in->sn_size += len;
    /* Add Your Code */
    fs_writeblock(fs, in->sn_directblock[0], (char *) data);
    fs_writeblock(fs,SSU_INODE_BLOCK,(char *)inodemap);

///
    //fs_writeblock(fs, SSU_DATA_BLOCK,(char *)data);

    return result;
}

int inode_read(struct inode *in, uint32_t offset, char *buf, int len) {//offset 0 64 128
    int result = 0;
    struct ssu_fs *fs = in->sn_fs;

    fs_readblock(fs, in->sn_directblock[0], tmpblock);
    memcpy(buf, tmpblock + offset, len);
    /* Add Your Code */

    return result;
}

static int sync_inode(struct ssu_fs *fs, struct inode *inode) {
    int result = 0;
    int offset = inode->sn_ino / INODE_PER_BLOCK; //offset to current inode index from SSU_INODE_BLOCK

    result = fs_writeblock(fs, SSU_INODE_BLOCK + offset, (char *) (inode_tbl + offset * INODE_PER_BLOCK));
    return result;
}

bool nameValidation(struct inode *cwd, char *name) {
    struct ssu_fs *fs = cwd->sn_fs;

    fs_readblock(fs, cwd->sn_directblock[0], tmpblock);
    struct direntry *dataBlock = (struct direntry *) tmpblock;
    struct inode *inode;

    for (int i = 0; i < num_direntry(cwd); i++) {
        if (strcmp(dataBlock[i].de_name, name) == 0) {
            printk("same name directory aleady exist\n");
            return false;
        }
    }
    return true;
}


int make_dir(struct inode *cwd, char *name) {
    /* Add Your Code */
    //printk("make_dir func income\n");
    struct ssu_fs *fs = cwd->sn_fs;
    struct bitmap *b = fs->fs_blkmap;
    int newBlockIndex;

    if (!nameValidation(cwd, name))
        return -1;
    balloc(b, &newBlockIndex);//newBlockIndex에는 새로운 datablock index


    //printk("newBlockIndex : %d\n", newBlockIndex);

    //printk("currentBlockIndex : %d\n", cwd->sn_directblock[0]);

    struct inode *new_inode = inode_create(fs, SSU_TYPE_DIR);//새로운 디렉토리 datablock의 inode

    struct direntry new_dir/* = {new_inode->sn_ino, name}*/;
    new_dir.de_ino = new_inode->sn_ino;
    memcpy(&new_dir.de_name, name, FILENAME_LEN);


    struct direntry *dataBlock = (struct direntry *) tmpblock;

    inode_write(cwd, num_direntry(cwd), (char *) &new_dir, sizeof(struct direntry));


    struct direntry cur_dir__ = {new_inode->sn_ino, "."};
    struct direntry parent_dir = {cwd->sn_ino, ".."};

    memcpy(&dataBlock[0], &cur_dir__, sizeof(struct direntry));
    memcpy(&dataBlock[1], &parent_dir, sizeof(struct direntry));

    inode_write(new_inode, num_direntry(new_inode), &cur_dir__, sizeof(struct direntry));
    inode_write(new_inode, num_direntry(new_inode),  &parent_dir, sizeof(struct direntry));
    sync_inode(fs, new_inode);
    return 0;
}

static int num_direntry(struct inode *in) {
    if (in->sn_size % sizeof(struct direntry) != 0 || in->sn_type != SSU_TYPE_DIR)
        return -1;

    return in->sn_size / sizeof(struct direntry);
}


void list_segment(struct inode *cwd) {
    /* Add Your Code */
    struct ssu_fs *fs = cwd->sn_fs;

    fs_readblock(fs, cwd->sn_directblock[0], tmpblock);
    struct direntry *dataBlock = (struct direntry *) tmpblock;
    struct inode *inode;
    printk("name | size | type | blocks | ino\n");
    for (int i = 0; i < num_direntry(cwd); i++) {
        inode = &inode_tbl[dataBlock[i].de_ino];

        switch (inode->sn_type) {
            printk("sn_tpye : %d\n", inode->sn_type);
            case 2:
                printk("%s | %d | d | %d | %d\n", dataBlock[i].de_name, inode->sn_size, inode->sn_nlink,
                       dataBlock[i].de_ino);
                break;
            case 1:
                printk("%s | %d | f | %d | %d\n", dataBlock[i].de_name, inode->sn_size, inode->sn_nlink,
                       dataBlock[i].de_ino);
                break;
        }


    }
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
        if (de->de_ino == cwd->sn_ino) {
            return 0;
        }

    }
    return -1;
}