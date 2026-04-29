#include <stdint.h>
#include <kernel/kmalloc.h>
#include <kernel/sbd.h>
#include <kernel/task.h>

struct Token {
    uint8_t can_access_root;
};

struct TokenDictionary{
    uint64_t id;
    struct Token token;
    struct TokenDictionary *next;
};

struct TokenDictionary *tok_dict = 0;

void *KernelGetTokenForProcess(uint64_t id, uint8_t permissions){
    struct TokenDictionary *new_td = malloc(sizeof(struct TokenDictionary));
    new_td->token.can_access_root = permissions & 1;
    new_td->id = id;
    if(tok_dict == 0){
        tok_dict = new_td;
        return &new_td->token;
    }
    struct TokenDictionary *traversal_dict = tok_dict;
    while(traversal_dict->next) traversal_dict = traversal_dict->next;
    traversal_dict->next = new_td;
    return &new_td->token;
}

int KernelCheckToken(uint8_t permission){
    uint64_t id = GetCurrentProcessId();
    if(!id) return 1;
    extern struct Task *find_process(uint64_t id);
    struct Task *task = find_process(id);
    if((((struct Token *)task->token)->can_access_root & permission) == permission) return 1;
    kprint("Not enough privilege");
    return 0;
}
