module tango.stdc.stat;

alias int dev_t;
alias int ino_t;
alias ushort mode_t;
alias ushort nlink_t;
alias ushort uid_t;
alias ushort gid_t;
alias int off_t;
alias uint time_t;

struct stat_t
{
    dev_t   st_dev;
    ino_t   st_ino;
    mode_t  st_mode;
    nlink_t st_nlink;
    uid_t   st_uid;
    gid_t   st_gid;
    off_t   st_size;
    ushort	st_blockSize;
    ushort	st_blockCount;
    time_t  st_atime;
    time_t  st_mtime;
    time_t  st_ctime;
}

const S_IRUSR = 0000400;
const S_IWUSR = 0000200;
const S_IXUSR = 0000100;
const S_IRWXU = S_IRUSR | S_IWUSR | S_IXUSR;

const S_IRGRP = 0000040;
const S_IWGRP = 0000020;
const S_IXGRP = 0000010;
const S_IRWXG = S_IRGRP | S_IWGRP | S_IXGRP;

const S_IROTH = 0000004;
const S_IWOTH = 0000002;
const S_IXOTH = 0000001;
const S_IRWXO = S_IROTH | S_IWOTH | S_IXOTH;

const S_ISUID = 0004000;
const S_ISGID = 0002000;
const S_ISVTX = 0001000;

const S_IFMT    = 0170000;
const S_IFBLK   = 0060000;
const S_IFCHR   = 0020000;
const S_IFIFO   = 0010000;
const S_IFREG   = 0100000;
const S_IFDIR   = 0040000;
const S_IFLNK   = 0120000;
const S_IFSOCK  = 0140000;

extern (C) int stat(char*, stat_t*);
extern (C) int fstat(int, stat_t*);

private
{
    extern (D) bool S_ISTYPE( mode_t mode, uint mask )
    {
        return ( mode & S_IFMT ) == mask;
    }
}

extern (D) bool S_ISBLK( mode_t mode )  { return S_ISTYPE( mode, S_IFBLK );  }
extern (D) bool S_ISCHR( mode_t mode )  { return S_ISTYPE( mode, S_IFCHR );  }
extern (D) bool S_ISDIR( mode_t mode )  { return S_ISTYPE( mode, S_IFDIR );  }
extern (D) bool S_ISFIFO( mode_t mode ) { return S_ISTYPE( mode, S_IFIFO );  }
extern (D) bool S_ISREG( mode_t mode )  { return S_ISTYPE( mode, S_IFREG );  }
extern (D) bool S_ISLNK( mode_t mode )  { return S_ISTYPE( mode, S_IFLNK );  }
extern (D) bool S_ISSOCK( mode_t mode ) { return S_ISTYPE( mode, S_IFSOCK ); }

extern bool S_TYPEISMQ( stat_t* buf )  { return false; }
extern bool S_TYPEISSEM( stat_t* buf ) { return false; }
extern bool S_TYPEISSHM( stat_t* buf ) { return false; }
