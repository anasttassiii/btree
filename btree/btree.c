#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "btree.h"

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * Выделяет память под узел, массивы ключей и детей
 * calloc обнуляет память, что гарантирует NULL-указатели для детей
 */
static BNode* create_node(bool is_leaf, int max_keys) {
    BNode* node = (BNode*)calloc(1, sizeof(BNode));  // calloc обнуляет все поля
    if (node == NULL) return NULL;

    node->keys = (int*)calloc(max_keys, sizeof(int));  // массив для ключей
    node->children = (BNode**)calloc(max_keys + 1, sizeof(BNode*));  // детей на 1 больше
    node->num_keys = 0;          // пока ключей нет
    node->is_leaf = is_leaf;     // лист или внутренний узел

    // Если память не выделилась — чистим всё и выходим
    if (node->keys == NULL || node->children == NULL) {
        free(node->keys);
        free(node->children);
        free(node);
        return NULL;
    }

    return node;
}

// Находит позицию для ключа в узле (первый ключ >= key)

static int find_key_pos(BNode* node, int key) {
    int pos = 0;
    while (pos < node->num_keys && node->keys[pos] < key) {
        pos++;
    }
    return pos;
}

/**
 * Проверяет, содержит ли узел ключ
 * Если ключ есть — возвращает true и записывает его позицию в *pos.
 * Если нет — false.
 */
static bool node_has_key(BNode* node, int key, int* pos) {
    *pos = find_key_pos(node, key);  // находим место
    return (*pos < node->num_keys && node->keys[*pos] == key);  // проверяем совпадение
}

/**
 * Разбивает заполненный дочерний узел
 * Когда узел переполнен (2t-1 ключей), он разбивается на два узла по t-1 ключей,
 * а средний ключ поднимается в родительский узел.
 */
static void btree_split_child(BTree* tree, BNode* parent, int child_idx) {
    int t = tree->t;                     // минимальная степень
    BNode* child = parent->children[child_idx];  // переполненный ребёнок
    BNode* new_child = create_node(child->is_leaf, tree->max_keys);  // новый узел
    if (new_child == NULL) return;

    // Шаг 1: копируем последние t-1 ключей из старого узла в новый
    new_child->num_keys = t - 1;
    for (int i = 0; i < t - 1; i++) {
        new_child->keys[i] = child->keys[i + t];  // берём вторую половину
    }

    // Шаг 2: если не лист — копируем соответствующих детей
    if (!child->is_leaf) {
        for (int i = 0; i < t; i++) {
            new_child->children[i] = child->children[i + t];
        }
    }

    // Шаг 3: в старом узле остаётся t-1 ключей (первая половина)
    child->num_keys = t - 1;

    // Шаг 4: сдвигаем детей родителя, чтобы освободить место для нового узла
    for (int i = parent->num_keys; i > child_idx; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[child_idx + 1] = new_child;

    // Шаг 5: сдвигаем ключи родителя и вставляем средний ключ
    for (int i = parent->num_keys - 1; i >= child_idx; i--) {
        parent->keys[i + 1] = parent->keys[i];
    }
    parent->keys[child_idx] = child->keys[t - 1];  // средний ключ поднимается вверх
    parent->num_keys++;
}

/**
 * Вставляет ключ в незаполненный узел (рекурсивно)
 * Гарантируется, что узел НЕ заполнен (num_keys < max_keys)
 * Идём рекурсивно вниз до листа, разбивая по пути переполненные узлы
 */
static void btree_insert_nonfull(BTree* tree, BNode* node, int key) {
    int pos = find_key_pos(node, key);

    // Если ключ уже есть — ничего не делаем (дубликаты не храним)
    if (pos < node->num_keys&& node->keys[pos] == key) {
        return;
    }

    // Если узел — лист, просто вставляем ключ
    if (node->is_leaf) {
        // Сдвигаем ключи вправо, освобождая место
        for (int i = node->num_keys; i > pos; i--) {
            node->keys[i] = node->keys[i - 1];
        }
        node->keys[pos] = key;
        node->num_keys++;
        return;
    }

    // Иначе спускаемся к ребёнку
    // Если ребёнок заполнен — сначала разбиваем его
    if (node->children[pos]->num_keys == tree->max_keys) {
        btree_split_child(tree, node, pos);
        // После разбиения определяем, куда произвести вставку
        if (key > node->keys[pos]) {
            pos++;  // идём в право
        }
    }

    // Рекурсивно производим вставвку
    btree_insert_nonfull(tree, node->children[pos], key);
}

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * Создаёт B-дерево с минимальной степенью t
 * t >= 2, иначе дерево не имеет смысла.
 * max_keys = 2t-1 (максимум ключей в узле)
 * min_keys = t-1 (минимум ключей, кроме корня)
 */
BTree* btree_create(int t) {
    if (t < 2) {
        fprintf(stderr, "ERROR: B-tree degree must be >= 2\n");
        return NULL;
    }

    BTree* tree = (BTree*)calloc(1, sizeof(BTree));
    if (tree == NULL) return NULL;

    tree->t = t;
    tree->max_keys = 2 * t - 1;
    tree->min_keys = t - 1;
    tree->root = create_node(true, tree->max_keys);  // корень — лист

    if (tree->root == NULL) {
        free(tree);
        return NULL;
    }

    return tree;
}

/**
 * Рекурсивно удаляет узел и все его поддеревья
 * Обход в глубину: сначала дети, потом сам узел.
 */
void btree_destroy_node(BNode* node) {
    if (node == NULL) return;

    // Если не лист — сначала удаляем всех детей
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            btree_destroy_node(node->children[i]);
        }
    }

    // Теперь удаляем сам узел
    free(node->keys);
    free(node->children);
    free(node);
}

/**
 * Удаляет всё дерево
 */
void btree_destroy(BTree* tree) {
    if (tree == NULL) return;
    btree_destroy_node(tree->root);
    free(tree);
}

/**
 *  Вставляет ключ в B-дерево
 * Особый случай: если корень заполнен — создаём новый корень,
 * разбиваем старый, и вставляем ключ в новый корень.
 */
bool btree_insert(BTree* tree, int key) {
    if (tree == NULL || tree->root == NULL) return false;

    BNode* root = tree->root;

    // Если корень заполнен — разбиваем его (увеличиваем высоту дерева)
    if (root->num_keys == tree->max_keys) {
        BNode* new_root = create_node(false, tree->max_keys);  // новый корень — не лист
        if (new_root == NULL) return false;

        new_root->children[0] = root;           // старый корень становится ребёнком
        tree->root = new_root;
        btree_split_child(tree, new_root, 0);   // разбиваем старого корня
        btree_insert_nonfull(tree, new_root, key);  // вставляем в новый корень
    }
    else {
        btree_insert_nonfull(tree, root, key);  // корень не заполнен — вставляем как обычно
    }

    return true;
}

/**
 * Ищет ключ в B-дереве
 * Спускается от корня к листьям, сравнивая ключи
 * Возвращает true и записывает значение в found_value (если указатель не NULL)
 */
bool btree_search(BTree* tree, int key, int* found_value) {
    if (tree == NULL || tree->root == NULL) return false;

    BNode* node = tree->root;

    while (node != NULL) {
        int pos = find_key_pos(node, key);

        // Нашли ключ в текущем узле
        if (pos < node->num_keys && node->keys[pos] == key) {
            if (found_value != NULL) {
                *found_value = node->keys[pos];
            }
            return true;
        }

        // Если лист — ключа нет
        if (node->is_leaf) {
            return false;
        }

        // Иначе спускаемся к нужному ребёнку
        node = node->children[pos];
    }

    return false;
}

/**
 * Удаляет ключ из B-дерева
 * Полная реализация удаления требует слияния узлов, здесь же упрощённая версия — только проверка существования
 */
bool btree_delete(BTree* tree, int key) {
    if (tree == NULL || tree->root == NULL) return false;

    BNode* node = tree->root;
    int pos;

    if (!node_has_key(node, key, &pos)) {
        return false;
    }

    // Для простоты показываем, что удаление НЕ реализовано полностью
    fprintf(stderr, "WARNING: Full delete not implemented in this demo\n");
    return false;
}

/**
 * Рекурсивно печатает узел и его поддеревья
 * level — глубина вложенности (для отступов)
 * Печатает ключи узла в формате [k1, k2, ...] и детей с отступами.
 */
void btree_print_node(BNode* node, int level) {
    if (node == NULL) return;

    // Отступы для визуализации уровня
    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    // Печатаем ключи узла
    printf("[");
    for (int i = 0; i < node->num_keys; i++) {
        printf("%d", node->keys[i]);
        if (i < node->num_keys - 1) printf(", ");
    }
    printf("] %s\n", node->is_leaf ? "(leaf)" : "");

    // Рекурсивно печатаем детей
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            btree_print_node(node->children[i], level + 1);
        }
    }
}

// Печатает всё B-дерево
void btree_print(BTree* tree) {
    if (tree == NULL || tree->root == NULL) {
        printf("Empty tree\n");
        return;
    }

    printf("B-Tree (t=%d, max_keys=%d):\n", tree->t, tree->max_keys);
    btree_print_node(tree->root, 0);
}

/**
 * Вычисляет высоту дерева (количество уровней)
 * Так как B-дерево идеально сбалансировано, высоту можно найти,
 * пройдя по левым детям до листа
 */
int btree_height(BTree* tree) {
    if (tree == NULL || tree->root == NULL) return 0;

    int height = 1;
    BNode* node = tree->root;

    // Идём по самым левым детям до листа
    while (!node->is_leaf) {
        node = node->children[0];
        height++;
    }

    return height;
}

// Подсчитывает общее количество ключей (рекурсивно)
static int count_keys_node(BNode* node) {
    if (node == NULL) return 0;

    int count = node->num_keys;  // ключи текущего узла

    // + ключи всех детей (если есть)
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            count += count_keys_node(node->children[i]);
        }
    }

    return count;
}

// Возвращает общее количество ключей в дереве

int btree_count_keys(BTree* tree) {
    if (tree == NULL || tree->root == NULL) return 0;
    return count_keys_node(tree->root);
}

// Проверяет, пустое ли дерево

bool btree_is_empty(BTree* tree) {
    return tree == NULL || tree->root == NULL || tree->root->num_keys == 0;
}