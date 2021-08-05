#include <stdio.h>
#include <stdlib.h>
#include "list.h"

struct Node {
    int num;
    struct list_head node;
};


int main(void)
{
    struct list_head head;

    /*init head*/
    INIT_LIST_HEAD(&head);


    /*malloc for new node*/
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->num = 111;

    /*add new node to head*/
    list_add_tail(&newNode->node, &head);

    /*malloc for new node*/
    struct Node *newNode2 = (struct Node *)malloc(sizeof(struct Node));
    newNode2->num = 222;

    list_add_tail(&newNode2->node, &head);

    struct Node *tmp;
    list_for_each_entry(tmp, &head, node){
        printf("memory address: %p\n", tmp);
        printf("node num:%d\n\n", tmp->num); 
    }

    struct Node *p = list_first_entry(&head, typeof(*tmp), node);
    printf("list_first_entry: %p\n", p);

    p =list_entry(head.next, typeof(*tmp), node);
    printf("list_entry: %p\n", p);
        
    struct Node *pNode = NULL;
    struct Node *pNextNode = NULL;

    list_for_each_entry_safe(pNode, pNextNode, &head, node){
        list_del(&pNode->node); 
        free(pNode);
    }

    printf("After list_del...\n");

    struct Node *tmp2;
    list_for_each_entry(tmp2, &head, node){
    
        printf("node num:%d\n", tmp2->num); 
    }

    return 0;
}
