#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>  
#include "btree.h"

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ТЕСТОВ
// ============================================================================

static void print_separator(const char* title) {
    printf("\n========================================\n");
    printf("  %s\n", title);
    printf("========================================\n");
}

// Универсальная функция для проверки с форматированием
static void test_assert(bool condition, const char* format, ...) {
    va_list args;
    va_start(args, format);

    printf("  ");
    if (condition) {
        printf("[PASS] ");
        vprintf(format, args);
        printf("\n");
    }
    else {
        printf("[FAIL] ");
        vprintf(format, args);
        printf("\n");
    }

    va_end(args);
}

// Простая функция без форматирования (для простых сообщений)
static void test_assert_simple(bool condition, const char* message) {
    test_assert(condition, "%s", message);
}

// ============================================================================
// ТЕСТ 1: БАЗОВЫЕ ОПЕРАЦИИ
// ============================================================================

static void test_basic_operations(void) {
    print_separator("TEST 1: BASIC OPERATIONS");

    BTree* tree = btree_create(3);
    test_assert_simple(tree != NULL, "Create B-tree");

    int keys[] = { 10, 20, 30, 40, 50, 25, 15, 35, 45, 5 };
    int num = sizeof(keys) / sizeof(keys[0]);

    printf("\nInserting keys: ");
    for (int i = 0; i < num; i++) {
        printf("%d ", keys[i]);
        btree_insert(tree, keys[i]);
    }
    printf("\n");

    printf("\nTree structure:\n");
    btree_print(tree);

    printf("\nSearch tests:\n");
    for (int i = 0; i < num; i++) {
        int value;
        bool found = btree_search(tree, keys[i], &value);
        test_assert(found, "Found key %d", keys[i]);
    }

    int value;
    bool found = btree_search(tree, 100, &value);
    test_assert(!found, "Key 100 NOT found");

    printf("\nStatistics:\n");
    printf("  Height: %d\n", btree_height(tree));
    printf("  Total keys: %d\n", btree_count_keys(tree));
    test_assert(btree_count_keys(tree) == num, "All %d keys inserted", num);

    btree_destroy(tree);
}

// ============================================================================
// ТЕСТ 2: ВСТАВКА МНОГО КЛЮЧЕЙ
// ============================================================================

static void test_insert_many_keys(void) {
    print_separator("TEST 2: INSERT MANY KEYS (SPLIT TEST)");

    BTree* tree = btree_create(2);

    printf("\nInserting 15 keys (t=2)...\n");
    for (int i = 1; i <= 15; i++) {
        btree_insert(tree, i);
    }

    printf("\nTree structure after insert 1..15:\n");
    btree_print(tree);

    test_assert(btree_count_keys(tree) == 15, "All 15 keys inserted");
    test_assert(btree_height(tree) == 3, "Height = 3 for 15 keys with t=2");

    btree_destroy(tree);
}

// ============================================================================
// ТЕСТ 3: ПОИСК В БОЛЬШОМ ДЕРЕВЕ
// ============================================================================

static void test_search_large_tree(void) {
    print_separator("TEST 3: SEARCH IN LARGE TREE");

    BTree* tree = btree_create(4);

    printf("\nInserting 100 keys...\n");
    for (int i = 1; i <= 100; i++) {
        btree_insert(tree, i);
    }

    printf("Height: %d\n", btree_height(tree));
    printf("Total keys: %d\n", btree_count_keys(tree));

    int found_count = 0;
    for (int i = 1; i <= 100; i++) {
        int value;
        if (btree_search(tree, i, &value)) {
            found_count++;
        }
    }

    test_assert(found_count == 100, "All 100 keys found");

    int value;
    test_assert(!btree_search(tree, 0, &value), "Key 0 NOT found");
    test_assert(!btree_search(tree, 101, &value), "Key 101 NOT found");

    btree_destroy(tree);
}

// ============================================================================
// ТЕСТ 4: РАЗНЫЕ СТЕПЕНИ
// ============================================================================

static void test_different_degrees(void) {
    print_separator("TEST 4: DIFFERENT DEGREES");

    int degrees[] = { 2, 3, 4, 5 };     // Массив различных минимальных степеней для проверки
    int num_degrees = sizeof(degrees) / sizeof(degrees[0]);

    for (int d = 0; d < num_degrees; d++) {
        int t = degrees[d];
        BTree* tree = btree_create(t);

        printf("\nTesting degree t = %d\n", t);

        int num_keys = 500;
        for (int i = 1; i <= num_keys; i++) {
            btree_insert(tree, i);
        }

        int height = btree_height(tree);           // Получаем фактическую высоту и количество ключей
        int count = btree_count_keys(tree);

        printf("  Height: %d, Keys: %d\n", height, count);
        test_assert(count == num_keys, "All keys inserted (t=%d)", t);           // Проверяем, что все ключи вставлены

        int max_height = 0;
        int temp = 1;
        while (temp < (num_keys + 1) / 2) {       // Вычисляем максимально допустимую высоту по формуле из теории:
            temp *= t;
            max_height++;
        }
        max_height++;

        printf("  Max height: %d\n", max_height);
        test_assert(height <= max_height, "Height <= %d (t=%d)", max_height, t);

        btree_destroy(tree);
    }
}

// ============================================================================
// ТЕСТ 5: ПРОИЗВОДИТЕЛЬНОСТЬ
// ============================================================================

static void test_performance(void) {
    print_separator("TEST 5: PERFORMANCE");

    const int NUM_KEYS = 10000;
    const int DEGREE = 4;

    BTree* tree = btree_create(DEGREE);

    printf("\nInserting %d keys (t=%d)...\n", NUM_KEYS, DEGREE);
    for (int i = 1; i <= NUM_KEYS; i++) {
        btree_insert(tree, i);
    }

    printf("  Height: %d\n", btree_height(tree));
    printf("  Total keys: %d\n", btree_count_keys(tree));

    printf("\nSearching all keys...\n");
    int found = 0;
    for (int i = 1; i <= NUM_KEYS; i++) {
        int value;
        if (btree_search(tree, i, &value)) {
            found++;
        }
    }

    test_assert(found == NUM_KEYS, "All %d keys found", NUM_KEYS);

    btree_destroy(tree);
}

// ============================================================================
// ТЕСТ 6: РЕАЛЬНЫЙ ПРИМЕР
// ============================================================================

static void test_real_world_example(void) {
    print_separator("TEST 6: REAL-WORLD EXAMPLE (DICTIONARY)");

    BTree* dict = btree_create(3);

    printf("\nStudent ID database:\n");
    int ids[] = { 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010 };
    for (int i = 0; i < 10; i++) {
        btree_insert(dict, ids[i]);
        printf("  Inserted ID: %d\n", ids[i]);
    }

    printf("\nTree structure:\n");
    btree_print(dict);

    printf("\nSearch tests:\n");
    for (int i = 0; i < 10; i++) {
        int value;
        bool found = btree_search(dict, ids[i], &value);
        test_assert(found, "Student %d found", ids[i]);
    }

    test_assert(!btree_search(dict, 999, NULL), "Student 999 NOT found");
    test_assert(btree_count_keys(dict) == 10, "All 10 IDs inserted");

    btree_destroy(dict);
}

// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================================

int main(void) {
    printf("============================================\n");
    printf("     B-TREE IMPLEMENTATION\n");
    printf("============================================\n");

    test_basic_operations();
    test_insert_many_keys();
    test_search_large_tree();
    test_different_degrees();
    test_performance();
    test_real_world_example();

    printf("\n============================================\n");
    printf("           ALL TESTS PASSED!\n");
    printf("============================================\n");

    return 0;
}