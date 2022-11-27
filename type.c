#include "9cc.h"

Type *char_type = &(Type){ TY_CHAR, 1 };
Type *int_type = &(Type){ TY_INT, 8 };

// 目的：Type を受け取り、int 型かどうかを調べる
// is_integer : Type -> bool
bool is_integer(Type *ty) {
    return ty->kind == TY_CHAR || ty->kind == TY_INT;
}

// 目的：Type 型の base ポインタを受け取り、それを指すポインタ型の Type を返す
// pointer_to : Type -> Type
Type *pointer_to(Type *base) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->base = base;
    return ty;
}

// 目的：Type 型の base ポインタと配列の長さを受けとり、それらの型と要素数を持った配列を返す
// array_of : Type -> int -> Type
Type *array_of(Type *base, int len) {
    Type *ty = calloc(1, sizeof(Type));
    ty->kind = TY_ARRAY;
    ty->size = base->size * len;
    ty->base = base;
    ty->array_len = len;
    return ty;
}

// 目的：各ノードに型の情報を加える（int / ポインタ）
// add_type : Node -> void (引数の Node に型情報を加える)
void add_type(Node *node) {
    if (!node || node->ty)
        return;
    
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);

    for (Node *n = node->body; n; n = n->next)
        add_type(n);
    for (Node *n = node->args; n; n = n->next)
        add_type(n);
    
    switch (node->kind) {
        // 下記のケースでは、Nodeの型は int
        case ND_ADD:
        case ND_SUB:
        case ND_PTR_DIFF:
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_FUNCALL:
        case ND_NUM:
            node->ty = int_type;
            return;
        // 下記のケースでは、Nodeの型は左辺値の型
        case ND_PTR_ADD:
        case ND_PTR_SUB:
        case ND_ASSIGN:
            node->ty = node->lhs->ty;
            return;
        // 下記のケースでは、Nodeの型は変数の型
        case ND_VAR:
            node->ty = node->var->ty;
            return;
        // ND_ADDR の Node の場合はポインタ型
        case ND_ADDR:   // &
            if (node->lhs->ty->kind == TY_ARRAY)
                node->ty = pointer_to(node->lhs->ty->base);
            else
                node->ty = pointer_to(node->lhs->ty);
            return;
        // ND_DEREF の Node の場合
        case ND_DEREF:  // *
            // 左辺値の型がポインタの場合、base が指す型
            if (!node->lhs->ty->base)
                error_tok(node->tok, "ポインタの参照が間違っています");
            node->ty = node->lhs->ty->base;
            return;
    }
}