/*
 * fs_explorer.c — File System Explorer Lab
 *
 * Usage: ./fs_explorer <part_number>
 *   0  (command-line exercises — see LAB.md)
 *   1  Inodes: the real identity of a file
 *   2  Hard links and soft links
 *   3  Reading directories
 *   4  File operations: the syscall interface
 *   5  VFS: everything is a file
 *
 * Compile: make
 *     or:  gcc -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -o fs_explorer fs_explorer.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/* ----------------------------------------------------------------
 * Test paths — everything lives under /tmp/fs_lab so we never
 * accidentally touch student files.
 * ---------------------------------------------------------------- */
#define TESTDIR   "/tmp/fs_lab"
#define TESTFILE  TESTDIR "/original.txt"
#define HARDLINK  TESTDIR "/hardlink.txt"
#define SOFTLINK  TESTDIR "/softlink.txt"
#define SUBDIR    TESTDIR "/subdir"
#define RAWFILE   TESTDIR "/rawio.dat"
#define SPARSE    TESTDIR "/sparse.dat"

/* ----------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------- */

/* Create the test environment from scratch. */
static void setup(void)
{
    /* Clean slate */
    system("rm -rf " TESTDIR);
    mkdir(TESTDIR, 0755);

    /* A test file with known content */
    FILE *f = fopen(TESTFILE, "w");
    if (!f) { perror("fopen"); exit(1); }
    fprintf(f, "Hello from the file system lab!\n");
    fclose(f);

    /* A subdirectory with a handful of files */
    mkdir(SUBDIR, 0755);
    for (int i = 0; i < 5; i++) {
        char path[256];
        snprintf(path, sizeof(path), SUBDIR "/file%d.txt", i);
        f = fopen(path, "w");
        fprintf(f, "Contents of file %d\n", i);
        fclose(f);
    }
}

/*
 * Print key stat fields for a path.
 * Uses lstat() so that symbolic links are NOT followed.
 */
static void print_stat(const char *path, const char *label)
{
    struct stat st;
    if (lstat(path, &st) < 0) {
        printf("  %-14s  stat failed: %s\n", label, strerror(errno));
        return;
    }
    const char *type = S_ISREG(st.st_mode)  ? "regular"   :
                       S_ISDIR(st.st_mode)  ? "directory" :
                       S_ISLNK(st.st_mode)  ? "symlink"   :
                       S_ISCHR(st.st_mode)  ? "char-dev"  :
                       S_ISBLK(st.st_mode)  ? "block-dev" : "other";
    printf("  %-14s  inode=%-8lu  links=%-2lu  size=%-6ld  type=%s\n",
           label,
           (unsigned long)st.st_ino,
           (unsigned long)st.st_nlink,
           (long)st.st_size,
           type);
}

/*
 * Read and print the contents of a file (up to 255 bytes).
 * Returns 0 on success, -1 on failure.
 */
static int read_file(const char *path, const char *label)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("  %-14s  open failed: %s\n", label, strerror(errno));
        return -1;
    }
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    /* Strip trailing newline for cleaner output */
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    fclose(f);
    printf("  %-14s  contents: \"%s\"\n", label, buf);
    return 0;
}

/* ================================================================
 * Part 1: Inodes: The Real Identity of a File
 * (Fully provided -- study this code, then answer the questions.)
 * ================================================================ */
static void part1(void)
{
    printf("=== Part 1: Inodes: The Real Identity of a File ===\n\n");

    struct stat st;
    if (stat(TESTFILE, &st) < 0) {
        perror("stat");
        return;
    }

    printf("File: %s\n\n", TESTFILE);
    printf("  Inode number:    %lu\n", (unsigned long)st.st_ino);
    printf("  Link count:      %lu\n", (unsigned long)st.st_nlink);
    printf("  Size (bytes):    %ld\n", (long)st.st_size);
    printf("  Block size:      %ld\n", (long)st.st_blksize);
    printf("  Blocks (512B):   %ld\n", (long)st.st_blocks);
    printf("  Permissions:     %04o\n", st.st_mode & 07777);
    printf("  Device:          %lu\n\n", (unsigned long)st.st_dev);

    /* Create a hard link and compare inodes */
    if (link(TESTFILE, HARDLINK) < 0) {
        perror("link");
        return;
    }

    printf("After creating hard link '%s':\n\n", HARDLINK);
    print_stat(TESTFILE, "original");
    print_stat(HARDLINK, "hardlink");

    struct stat st2;
    stat(HARDLINK, &st2);
    printf("\n  Same inode? %s  (original=%lu, hardlink=%lu)\n",
           st.st_ino == st2.st_ino ? "YES" : "NO",
           (unsigned long)st.st_ino, (unsigned long)st2.st_ino);
    printf("  Link count now: %lu\n", (unsigned long)st2.st_nlink);

    /* Remove the hard link */
    unlink(HARDLINK);

    printf("\nAfter removing hard link:\n\n");
    stat(TESTFILE, &st);
    print_stat(TESTFILE, "original");
    printf("  Link count back to: %lu\n", (unsigned long)st.st_nlink);
}

/* ================================================================
 * Part 2: Hard Links and Soft Links
 * ================================================================ */
static void part2(void)
{
    printf("=== Part 2: Hard Links and Soft Links ===\n\n");

    printf("Original file:\n");
    print_stat(TESTFILE, "original");
    read_file(TESTFILE, "original");
    printf("\n");

    /* ----------------------------------------------------------
     * TODO Step 1: Create a hard link
     *
     * Use link(TESTFILE, HARDLINK) to create a hard link.
     * Then use print_stat() on both TESTFILE and HARDLINK.
     * Print a blank line after for readability.
     * ---------------------------------------------------------- */
    if (link (TESTFILE, HARDLINK) < 0) {
        perror("link");
        return;
    }

    printf("after creating hard link: \n");
    print_stat(TESTFILE, "original");
    print_stat(HARDLINK, "hard link");
    printf("\n");

    /* ----------------------------------------------------------
     * TODO Step 2: Create a symbolic (soft) link
     *
     * Use symlink(TESTFILE, SOFTLINK) to create a symbolic link.
     * Then use print_stat() on SOFTLINK.
     * Note: print_stat() uses lstat(), so you'll see the symlink
     * itself, not the target.  Compare the inode to the original.
     * Print a blank line after for readability.
     * ---------------------------------------------------------- */

    if (symlink(TESTFILE, SOFTLINK) < 0 ){
        perror("symlink");
        return;
    }

    printf(" after creating the soft linke: \n");
    print_stat(SOFTLINK, "softlink");
    printf("\n");

    /* ----------------------------------------------------------
     * TODO Step 3: Delete the original file and test both links
     *
     * 1. Delete the original file: unlink(TESTFILE)
     * 2. Print a header like "After deleting the original:\n"
     * 3. Try to read through the hard link: read_file(HARDLINK, ...)
     * 4. Try to read through the soft link: read_file(SOFTLINK, ...)
     * 5. Stat both links with print_stat()
     * ---------------------------------------------------------- */
    

    /* --- Clean up --- */
    unlink(TESTFILE);
    printf(" after deleting the original file: \n");
    read_file(HARDLINK, "hardlink");
    read_file(SOFTLINK, "softlink");
    print_stat(HARDLINK, "hardlink");
    print_stat(SOFTLINK, "softlink");

}

/* ================================================================
 * Part 3: Reading Directories
 * ================================================================ */
static void part3(void)
{
    printf("=== Part 3: Reading Directories ===\n\n");

    printf("Listing contents of: %s\n\n", SUBDIR);

    /* ----------------------------------------------------------
     * TODO: List the directory using opendir / readdir / closedir
     *
     * 1. Open the directory with opendir(SUBDIR).
     *    If it fails, print an error with perror() and return.
     *
     * 2. Loop over entries with readdir():
     *    - Print each entry's name (entry->d_name).
     *    - Build the full path with snprintf:
     *        char fullpath[512];
     *        snprintf(fullpath, sizeof(fullpath),
     *                 "%s/%s", SUBDIR, entry->d_name);
     *    - Call print_stat(fullpath, entry->d_name).
     *    - Keep a running count of entries.
     *
     * 3. Close the directory with closedir().
     *
     * 4. Print the total number of entries found.
     *
     * Hint: readdir() returns NULL when there are no more entries.
     * ---------------------------------------------------------- */
    DIR *dir = opendir(SUBDIR);
    if (!dir) {
        perror("opendir");
        return;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char absolutePath[512];
        snprintf(absolutePath, sizeof(absolutePath), "%s/%s", SUBDIR, entry->d_name);
        print_stat(absolutePath, entry->d_name);
        count++;
    }

    closedir(dir);
    printf("Total entries: %d\n", count);

    /* --- Provided: examine . and .. --- */

    printf("\nExamining '.' and '..' in %s:\n\n", SUBDIR);

    struct stat st_dot, st_dotdot, st_subdir, st_testdir;
    stat(SUBDIR "/.",  &st_dot);
    stat(SUBDIR "/..", &st_dotdot);
    stat(SUBDIR,       &st_subdir);
    stat(TESTDIR,      &st_testdir);

    printf("  %-18s  inode = %lu\n", SUBDIR,       (unsigned long)st_subdir.st_ino);
    printf("  %-18s  inode = %lu\n", SUBDIR "/.",   (unsigned long)st_dot.st_ino);
    printf("  %-18s  inode = %lu\n", SUBDIR "/..",  (unsigned long)st_dotdot.st_ino);
    printf("  %-18s  inode = %lu\n", TESTDIR,       (unsigned long)st_testdir.st_ino);
}

/* ================================================================
 * Part 4: File Operations: The Syscall Interface
 * ================================================================ */
static void part4(void)
{
    printf("=== Part 4: File Operations: The Syscall Interface ===\n\n");

    /* --- Experiment A: Sequential I/O with raw syscalls --- */

    printf("Experiment A: Sequential write / read with syscalls\n\n");

    /* ----------------------------------------------------------
     * TODO: Perform sequential file I/O using POSIX syscalls only
     *       (open, write, lseek, read, close — NOT fopen/fwrite).
     *
     * 1. Open RAWFILE for writing (create + truncate):
     *      int fd = open(RAWFILE,
     *                    O_CREAT | O_WRONLY | O_TRUNC, 0644);
     *    Check for errors (fd < 0).
     *
     * 2. Write the string "ABCDEFGH" (8 bytes) to the file.
     *
     * 3. Print the current offset using lseek(fd, 0, SEEK_CUR).
     *    (This queries the position without moving it.)
     *
     * 4. Close the file.
     *
     * 5. Re-open RAWFILE for reading:
     *      fd = open(RAWFILE, O_RDONLY);
     *
     * 6. Read the first 4 bytes into a char buf[5] (and
     *    null-terminate: buf[4] = '\0').  Print them.
     *
     * 7. Print the offset again (should be 4).
     *
     * 8. Use lseek(fd, 2, SEEK_SET) to jump to byte 2.
     *    Read 4 bytes again and print them. What did you get?
     *
     * 9. Close the file.
     * ---------------------------------------------------------- */

    int fileDescriptor = open(RAWFILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fileDescriptor < 0) {
        perror("open");
        return;
    }

    write(fileDescriptor, "ABCDEFGH", 8);
    off_t position = lseek(fileDescriptor, 0, SEEK_CUR);
    printf("after writing \"ABCDEFGH\": offset = %ld\n", (long)position);
    close(fileDescriptor);

    fileDescriptor = open(RAWFILE, O_RDONLY);
    if (fileDescriptor < 0) {
        perror("open");
        return;
    }

    char buf[5];
    read(fileDescriptor, buf, 4);
    buf[4] = '\0';
    position = lseek(fileDescriptor, 0, SEEK_CUR);
    printf("Read 4 bytes from start: \"%s\"  offset = %ld\n", buf, (long)position);

    lseek(fileDescriptor, 2, SEEK_SET);
    read(fileDescriptor, buf, 4);
    buf[4] = '\0';
    printf("After seeking to byte 2: \"%s\"\n", buf);

    close(fileDescriptor);

    printf("\n");

    /* --- Experiment B: Sparse files --- */

    printf("Experiment B: Sparse files (holes)\n\n");

    /* ----------------------------------------------------------
     * TODO: Create a sparse file by seeking past the end.
     *
     * 1. Open SPARSE for writing (create + truncate).
     *
     * 2. Write the byte 'A' at offset 0.
     *
     * 3. Seek to offset 1048576 (1 MiB) using SEEK_SET.
     *
     * 4. Write the byte 'Z' at that position.
     *
     * 5. Close the file.
     *
     * 6. Use stat() to get the file's st_size and st_blocks.
     *    Print both values.  Remember that st_blocks counts
     *    512-byte units.
     *    Compute and print the actual bytes on disk:
     *      st_blocks * 512
     *
     * 7. Re-open SPARSE for reading.  For each of the three offsets
     *    below, use lseek(fd, offset, SEEK_SET) to seek there, then
     *    read one byte:
     *      - offset 0       (should be 'A')
     *      - offset 500     (inside the hole)
     *      - offset 1048576 (should be 'Z')
     *    Print each result in the format:  'A' (65)
     *    For 'A' and 'Z' the char and its ASCII value; for the hole
     *    byte, just print its integer value -- it should be 0.
     *
     * 8. Close the file.
     * ---------------------------------------------------------- */

     int fileDescriptor2 = open(SPARSE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
     if (fileDescriptor2 < 0){
        perror("open");
        return;
     }

     write(fileDescriptor2, "A", 1);
     lseek(fileDescriptor2, 1048576, SEEK_SET);
     write(fileDescriptor2, "Z", 1);
     close(fileDescriptor2);

     struct stat st;
     stat(SPARSE, &st);
     printf("File size: %ld bytes\n", (long)st.st_size);
     printf("Blocks (512B): %ld\n", (long)st.st_blocks);
     printf("Actual disk use: %ld bytes\n", (long)st.st_blocks * 512);

     fileDescriptor2 = open(SPARSE, O_RDONLY);
     char c;
     lseek(fileDescriptor2, 0, SEEK_SET);
     read(fileDescriptor2, &c, 1);
     printf("Byte at offset 0: '%c' (%d)\n", c, c);

     lseek(fileDescriptor2, 500, SEEK_SET);
     read(fileDescriptor2, &c, 1);
     printf("Byte at offset 500: %d\n", c);

     lseek(fileDescriptor2, 1048576, SEEK_SET);
     read(fileDescriptor2, &c, 1);
     printf("Byte at offset 1048576: '%c' (%d)\n", c, c);

     close(fileDescriptor2);


}

/* ================================================================
 * Part 5: VFS: Everything Is a File
 * ================================================================ */
static void part5(void)
{
    printf("=== Part 5: VFS: Everything Is a File ===\n\n");

    /*
     * We will call the exact same syscalls — stat() and read() —
     * on four very different objects:
     *
     *   1. A regular file         (TESTFILE)
     *   2. A directory            (SUBDIR)
     *   3. A /proc entry          (/proc/self/status)
     *   4. A character device     (/dev/zero)
     *
     * The point: one interface, many back-ends.
     */


    const char *paths[] = {
        TESTFILE,
        SUBDIR,
        "/proc/self/status",
        "/dev/zero",
    };
    const char *labels[] = {
        "regular file",
        "directory",
        "/proc entry",
        "char device",
    };
    int n = 4;

    printf("--- stat() results ---\n\n");

    for (int i = 0; i < n; i++)
        print_stat(paths[i], labels[i]);

    printf("\n--- Reading a few bytes from each ---\n\n");

    /* ----------------------------------------------------------
     * TODO: For each of the four paths above, attempt to open()
     *       and read() a small amount of data.
     *
     * Loop over each path (i = 0 .. n-1):
     *
     *   1. Open the path with open(paths[i], O_RDONLY).
     *      If the open fails, print the label and the error
     *      with strerror(errno) and continue to the next path.
     *
     *   2. Read up to 64 bytes into a buffer.
     *      If read() returns < 0, print the label and the error
     *      with strerror(errno), close the fd, and continue.
     *      Note: on Linux, open() on a directory succeeds --
     *      only the read() call fails (with EISDIR).  That is
     *      the one case you should expect to hit here.
     *
     *   3. If the read succeeds (returns > 0):
     *      - For /dev/zero (i == 3): print the integer value
     *        of the first byte (should be 0).
     *      - For everything else: null-terminate the buffer
     *        and print the first line of text.  You can cut
     *        the buffer at the first '\n':
     *          char *nl = strchr(buf, '\n');
     *          if (nl) *nl = '\0';
     *        Then print it.
     *
     *   4. Close the fd.
     *
     * The goal: observe that read() works on a regular file,
     * works on /proc, works on /dev/zero, but fails on a
     * directory.  Same syscall, very different data sources.
     * ---------------------------------------------------------- */
    
     for (int i = 0; i < n; i++) {
        int fileDescriptor = open(paths[i], O_RDONLY);
        if (fileDescriptor < 0) {
            printf("  %-14s  open failed: %s\n", labels[i], strerror(errno));
            continue;
        }

        char buf[65];
        ssize_t nread = read(fileDescriptor, buf, 64);
        if (nread < 0) {
            printf("  %-14s  read failed: %s\n", labels[i], strerror(errno));
            close(fileDescriptor);
            continue;
        }

        if (i == 3) {
            printf("  %-14s  first byte = %d\n", labels[i], (unsigned char)buf[0]);
        } else {
            buf[nread] = '\0';
            char *nl = strchr(buf, '\n');
            if(nl){
                *nl = 0;
            }
            printf("  %-14s  \"%s\"\n", labels[i], buf);
        }

        close(fileDescriptor);
    }

}

/* ================================================================
 * main
 * ================================================================ */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <part_number>\n", argv[0]);
        fprintf(stderr, "  0  (command-line exercises — see LAB.md)\n");
        fprintf(stderr, "  1  Inodes: the real identity of a file\n");
        fprintf(stderr, "  2  Hard links and soft links\n");
        fprintf(stderr, "  3  Reading directories\n");
        fprintf(stderr, "  4  File operations: the syscall interface\n");
        fprintf(stderr, "  5  VFS: everything is a file\n");
        return 1;
    }

    int part = atoi(argv[1]);

    setup();

    switch (part) {
    case 1: part1(); break;
    case 2: part2(); break;
    case 3: part3(); break;
    case 4: part4(); break;
    case 5: part5(); break;
    default:
        fprintf(stderr, "Unknown part: %d (valid: 1-5)\n", part);
        return 1;
    }

    return 0;
}
