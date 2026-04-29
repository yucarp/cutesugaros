#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/sbd.h>
#include <kernel/task.h>
#include <kernel/object/directory.h>
#include <kernel/object/object.h>

void str_slice(char *string, int start, int end, char *result){
    int n = 0;
    while(start <= end){
        result[n++] = string[start++];
    }
}

void DirectoryAddChild(struct Directory *directory, struct ObjectHeader *object){
    if(!directory->header_list){
        directory->header_list = malloc(sizeof(struct ObjectHeaderList));
        directory->header_list->item = object;
        return;
    }

    struct ObjectHeaderList *traversal_obj = directory->header_list;

    while(traversal_obj->next) traversal_obj = traversal_obj->next;
    traversal_obj->next = malloc(sizeof(struct ObjectHeaderList));
    traversal_obj->next->item = object;
}

void DirectoryRemoveChild(struct Directory *directory, struct ObjectHeader *object){
    if(!directory->header_list) return;

    struct ObjectHeaderList *traversal_obj = directory->header_list;

    if(traversal_obj->item == object){
        struct ObjectHeaderList *old_element = directory->header_list;
        directory->header_list = directory->header_list->next;
        free(old_element);
    }

    while(traversal_obj->next){
        if(traversal_obj->next->item == object){
            struct ObjectHeaderList *old_element = traversal_obj->next;
            traversal_obj->next = traversal_obj->next->next;
            free(old_element);
        }
        traversal_obj = traversal_obj->next;
    }
}

struct Directory *CreateDirectory(struct Directory *root, char *name){
    struct Directory *dir = malloc(sizeof(struct Directory));
    dir->header.type = 1;
    memcpy(dir->header.name, name, 32);
    DirectoryAddChild(root, (void *) dir);
    return dir;
}

struct ObjectHeader *ResolveObjectName(struct Directory *root, char *name){
    if(!name) return 0;
    if(!root) {root = GetCurrentProcess()->cwd;}
    char obj_name[32] = {0};
    char remaining[256] = {0};

    int end = 0;
    for(; name[end] != '/'; ++end){
        if(end == 32) return 0;
        if(!name[end]) break;
    }

    str_slice(name, 0, end - 1, obj_name);
    str_slice(name, end + 1, strlen(name), remaining);

    if(!strncmp(obj_name, ".", 32)) {
        return ResolveObjectName(ObjGetRootObject(), remaining);
    }

    struct ObjectHeaderList *traversal = root->header_list;

    while(traversal){
        if(!strncmp(traversal->item->name, obj_name, 32)){
            break;
        }
        traversal = traversal->next;
    }

    if (!traversal){
        return 0;
    }

    if(remaining[0]){
        if(traversal->item->type == 1) return ResolveObjectName((struct Directory *)traversal->item, remaining);
    }

    return traversal->item;
}

int ChangeWorkingDirectory(struct Directory *root, char *name){
    struct Directory *dir = (void *) ResolveObjectName(root, name);
    if(dir) {GetCurrentProcess()->cwd = dir; return 0;}
    else return -1;
}
