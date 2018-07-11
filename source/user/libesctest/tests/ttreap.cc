/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/col/treap.h>
#include <sys/common.h>
#include <sys/test.h>
#include <stdlib.h>

#define TEST_NODE_COUNT     10

static void test_treap();
static void test_in_order();
static void test_rev_order();
static void test_rand_order();
static void test_add_and_rem(int *vals);

sTestModule tModTreap = {
    "Treap",test_treap,
};

struct MyNode : public esc::TreapNode<int> {
    MyNode(int k,int _data) : esc::TreapNode<int>(k),data(_data) {
    }
    int data;
};

static void test_treap() {
    test_in_order();
    test_rev_order();
    test_rand_order();
}

static void test_in_order() {
    test_caseStart("Add and remove nodes with increasing values");

    static int vals[TEST_NODE_COUNT];
    for(size_t i = 0; i < TEST_NODE_COUNT; i++)
        vals[i] = i;
    test_add_and_rem(vals);

    test_caseSucceeded();
}

static void test_rev_order() {
    test_caseStart("Add and remove nodes with decreasing values");

    static int vals[TEST_NODE_COUNT];
    for(size_t i = 0; i < TEST_NODE_COUNT; i++)
        vals[i] = TEST_NODE_COUNT - i;
    test_add_and_rem(vals);

    test_caseSucceeded();
}

static void test_rand_order() {
    test_caseStart("Add and remove nodes with addresses in rand order");

    static int vals[TEST_NODE_COUNT];
    for(size_t i = 0; i < TEST_NODE_COUNT; i++)
        vals[i] = i;
    srand(0x12345);
    for(size_t i = 0; i < 10000; i++) {
        size_t j = rand() % TEST_NODE_COUNT;
        size_t k = rand() % TEST_NODE_COUNT;
        uintptr_t t = vals[j];
        vals[j] = vals[k];
        vals[k] = t;
    }
    test_add_and_rem(vals);

    test_caseSucceeded();
}

static void test_add_and_rem(int *vals) {
    static MyNode *nodes[TEST_NODE_COUNT];
    esc::Treap<MyNode> tree;
    MyNode *node;

    // create
    for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
        nodes[i] = new MyNode(vals[i],i);
        tree.insert(nodes[i]);
    }

    // find all
    for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
        node = tree.find(vals[i]);
        test_assertPtr(node,nodes[i]);
    }

    // remove
    for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
        tree.remove(nodes[i]);
        node = tree.find(vals[i]);
        test_assertPtr(node,static_cast<MyNode*>(nullptr));
        delete nodes[i];

        for(size_t j = i + 1; j < TEST_NODE_COUNT; j++) {
            node = tree.find(vals[j]);
            test_assertPtr(node,nodes[j]);
        }
    }
}
