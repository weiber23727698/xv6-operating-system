#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

int dir, file, level = 0, error = 0, flag[6]={0};
char buf[512];

char choice(int sign){
    if(sign == 0)   return '|';
    return ' ';
}

void format(char *name){
    printf("%c", choice(flag[1]));
    for(int i = 1;i < level;i++)
        printf("   %c", choice(flag[i+1]));
    printf("\n");
    for(int i = 1;i < level;i++)
        printf("%c   ", choice(flag[i]));
    printf("+-- %s\n", name);
}

void tree(char *path){
    char *p;
    int fd = open(path, 0);
    if(fd < 0){
        error = 1;
        return;
    }
    struct stat st;
    fstat(fd, &st);
    if(st.type == T_FILE){
        error = 1;
        return;
    }
    if(level==0)    printf("\n");
    level++;
    struct dirent de;
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    read(fd, &de, sizeof(de));
    int cond = 0, val;
    while(1){
        if(strcmp(de.name, "..")==0 || strcmp(de.name, ".")==0){
            val = read(fd, &de, sizeof(de));
            cond = (val==0 || strlen(de.name)==0);
            if(cond)    break;
            continue;
        }
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        format(de.name);
        val = read(fd, &de, sizeof(de));
        cond = (val==0 || strlen(de.name)==0);
        stat(buf, &st);
        if(st.type == T_DIR){
            if(cond)
                flag[level] = 1;
            if(level > 0)    dir++;
            tree(buf);
            flag[level] = 0;
        }
        else
            file++;
        if(cond)    break;
    }
    if(level > 0)
        level--;
    close(fd);
}

int main(int argc, char *argv[]) {
    int fd[2];
    pipe(fd);
    if(fork() == 0){// child
        file = 0, dir = 0;
        printf("%s", argv[1]);
        tree(argv[1]);
        if(error == 1)
            printf(" [error opening dir]\n");
        printf("\n");
        close(fd[0]);
        write(fd[1], &dir, sizeof(int));
        write(fd[1], &file, sizeof(int));
        close(fd[1]);
    }
    else{// parent
        int file_num, dir_num, null;
        close(fd[1]);
        read(fd[0], &dir_num, sizeof(int));
        read(fd[0], &file_num, sizeof(int));
        close(fd[0]);
        printf("%d directories, %d files\n", dir_num, file_num);
        wait(&null);
    }
    exit(0);
}
