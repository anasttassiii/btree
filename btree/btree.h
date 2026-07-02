#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stddef.h>

// Структура узла B-дерева
typedef struct BNode {
    int* keys;              // Массив ключей
    struct BNode** children; // Массив указателей на детей
    int num_keys;           // Текущее количество ключей
    bool is_leaf;           // true если узел - лист
} BNode;


// Структура B-дерева
typedef struct {
    BNode* root;            // Корень дерева
    int t;                  // Минимальная степень (порядок дерева)
    int max_keys;           // 2*t - 1 (максимум ключей в узле)
    int min_keys;           // t - 1 (минимум ключей в узле, кроме корня)
} BTree;


// Создание и удаление
BTree* btree_create(int t);
void btree_destroy(BTree* tree);
void btree_destroy_node(BNode* node);


// Основные операции
bool btree_insert(BTree* tree, int key);
bool btree_search(BTree* tree, int key, int* found_value);
bool btree_delete(BTree* tree, int key);
void btree_print(BTree* tree);
void btree_print_node(BNode* node, int level);


// Вспомогательные функции
int btree_height(BTree* tree);
int btree_count_keys(BTree* tree);
bool btree_is_empty(BTree* tree);

#endif /* BTREE_H */