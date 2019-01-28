// Modifications copyright (C) 2017, Baidu.com, Inc.
// Copyright 2017 The Apache Software Foundation

// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef BDG_PALO_BE_SRC_EXPRS_ARITHMETIC_EXPR_H
#define BDG_PALO_BE_SRC_EXPRS_ARITHMETIC_EXPR_H

#include "exprs/expr.h"

namespace palo {

class ArithmeticExpr : public Expr {
public:
    static Expr* from_thrift(const TExprNode& node);
protected:
    enum BinaryOpType {
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        BIT_AND,
        BIT_OR,
        BIT_XOR,
        BIT_NOT,
    };

    ArithmeticExpr(const TExprNode& node) : Expr(node) { }
    virtual ~ArithmeticExpr() { }

    Status codegen_binary_op(
        RuntimeState* state, llvm::Function** fn, BinaryOpType op_type);
};

class AddExpr : public ArithmeticExpr {
public:
    AddExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~AddExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new AddExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
    virtual FloatVal get_float_val(ExprContext* context, TupleRow*);
    virtual DoubleVal get_double_val(ExprContext* context, TupleRow*);

    std::string debug_string() const override {
        return "(addExpr)" ;
    }
};

class SubExpr : public ArithmeticExpr {
public:
    SubExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~SubExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new SubExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
    virtual FloatVal get_float_val(ExprContext* context, TupleRow*);
    virtual DoubleVal get_double_val(ExprContext* context, TupleRow*);
    std::string debug_string() const override {
        return "(SubExpr)" ;
    }

};

class MulExpr : public ArithmeticExpr {
public:
    MulExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~MulExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new MulExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
    virtual FloatVal get_float_val(ExprContext* context, TupleRow*);
    virtual DoubleVal get_double_val(ExprContext* context, TupleRow*);
};

class DivExpr : public ArithmeticExpr {
public:
    DivExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~DivExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new DivExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
    virtual FloatVal get_float_val(ExprContext* context, TupleRow*);
    virtual DoubleVal get_double_val(ExprContext* context, TupleRow*);
};

class ModExpr : public ArithmeticExpr {
public:
    ModExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~ModExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new ModExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
    virtual FloatVal get_float_val(ExprContext* context, TupleRow*);
    virtual DoubleVal get_double_val(ExprContext* context, TupleRow*);
};

class BitAndExpr : public ArithmeticExpr {
public:
    BitAndExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~BitAndExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new BitAndExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
};

class BitOrExpr : public ArithmeticExpr {
public:
    BitOrExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~BitOrExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new BitOrExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
};

class BitXorExpr : public ArithmeticExpr {
public:
    BitXorExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~BitXorExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new BitXorExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
};

class BitNotExpr : public ArithmeticExpr {
public:
    BitNotExpr(const TExprNode& node) : ArithmeticExpr(node) { }
    virtual ~BitNotExpr() { }
    virtual Expr* clone(ObjectPool* pool) const override { 
        return pool->add(new BitNotExpr(*this)); 
    }
    virtual Status get_codegend_compute_fn(RuntimeState* state, llvm::Function** fn);
    virtual TinyIntVal get_tiny_int_val(ExprContext* context, TupleRow*);
    virtual SmallIntVal get_small_int_val(ExprContext* context, TupleRow*);
    virtual IntVal get_int_val(ExprContext* context, TupleRow*);
    virtual BigIntVal get_big_int_val(ExprContext* context, TupleRow*);
    virtual LargeIntVal get_large_int_val(ExprContext* context, TupleRow*);
};

}

#endif
