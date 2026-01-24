/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Code Generator Completo
 */

#include "codegen.h"
#include "../vm/bytecode.h"
#include <iostream>

namespace Kava {

std::vector<int32_t> Codegen::generate(const Program& program) {
    bytecode.clear();
    variables.clear();
    nextVarIdx = 0;
    
    // Gera código para classes
    for (auto& cls : program.classes) {
        // TODO: Implementar geração de código para classes
    }
    
    // Gera código para statements de nível superior
    for (auto& stmt : program.statements) {
        visitStatement(stmt);
    }
    
    emit(OP_HALT);
    return bytecode;
}

void Codegen::visitStatement(StmtPtr stmt) {
    if (!stmt) return;
    
    switch (stmt->getType()) {
        case NodeType::VarDecl: {
            auto varDecl = std::static_pointer_cast<VarDeclStmt>(stmt);
            if (varDecl->initializer) {
                visitExpression(varDecl->initializer);
            } else {
                emit(OP_PUSH_NULL);
            }
            variables[varDecl->name] = nextVarIdx++;
            emit(OP_STORE_GLOBAL);
            emit(variables[varDecl->name]);
            break;
        }
        
        case NodeType::PrintStmt: {
            auto printStmt = std::static_pointer_cast<PrintStmt>(stmt);
            visitExpression(printStmt->expression);
            emit(OP_PRINT);
            break;
        }
        
        case NodeType::IfStmt: {
            auto ifStmt = std::static_pointer_cast<IfStmt>(stmt);
            visitExpression(ifStmt->condition);
            emit(OP_JZ);
            int jzAddr = bytecode.size();
            emit(0); // Placeholder
            
            visitStatement(ifStmt->thenBranch);
            
            if (ifStmt->elseBranch) {
                emit(OP_JMP);
                int jmpAddr = bytecode.size();
                emit(0); // Placeholder
                bytecode[jzAddr] = bytecode.size();
                visitStatement(ifStmt->elseBranch);
                bytecode[jmpAddr] = bytecode.size();
            } else {
                bytecode[jzAddr] = bytecode.size();
            }
            break;
        }
        
        case NodeType::WhileStmt: {
            auto whileStmt = std::static_pointer_cast<WhileStmt>(stmt);
            int startAddr = bytecode.size();
            visitExpression(whileStmt->condition);
            emit(OP_JZ);
            int jzAddr = bytecode.size();
            emit(0); // Placeholder
            
            visitStatement(whileStmt->body);
            emit(OP_JMP);
            emit(startAddr);
            bytecode[jzAddr] = bytecode.size();
            break;
        }
        
        case NodeType::DoWhileStmt: {
            auto doWhile = std::static_pointer_cast<DoWhileStmt>(stmt);
            int startAddr = bytecode.size();
            visitStatement(doWhile->body);
            visitExpression(doWhile->condition);
            emit(OP_JNZ);
            emit(startAddr);
            break;
        }
        
        case NodeType::ForStmt: {
            auto forStmt = std::static_pointer_cast<ForStmt>(stmt);
            
            // Init
            for (auto& initStmt : forStmt->init) {
                visitStatement(initStmt);
            }
            
            int condAddr = bytecode.size();
            
            // Condition
            if (forStmt->condition) {
                visitExpression(forStmt->condition);
                emit(OP_JZ);
            } else {
                emit(OP_PUSH_TRUE);
                emit(OP_JZ);
            }
            int exitAddr = bytecode.size();
            emit(0);
            
            // Body
            visitStatement(forStmt->body);
            
            // Update
            for (auto& updateExpr : forStmt->update) {
                visitExpression(updateExpr);
                emit(OP_POP);
            }
            
            emit(OP_JMP);
            emit(condAddr);
            
            bytecode[exitAddr] = bytecode.size();
            break;
        }
        
        case NodeType::Block: {
            auto blockStmt = std::static_pointer_cast<BlockStmt>(stmt);
            for (auto& s : blockStmt->statements) {
                visitStatement(s);
            }
            break;
        }
        
        case NodeType::ReturnStmt: {
            auto returnStmt = std::static_pointer_cast<ReturnStmt>(stmt);
            if (returnStmt->value) {
                visitExpression(returnStmt->value);
                emit(OP_IRET);
            } else {
                emit(OP_RET);
            }
            break;
        }
        
        case NodeType::BreakStmt: {
            // TODO: Implementar break com label
            emit(OP_JMP);
            // Será preenchido depois (precisa de stack de loops)
            emit(0);
            break;
        }
        
        case NodeType::ContinueStmt: {
            // TODO: Implementar continue com label
            emit(OP_JMP);
            emit(0);
            break;
        }
        
        case NodeType::TryStmt: {
            auto tryStmt = std::static_pointer_cast<TryStmt>(stmt);
            
            emit(OP_TRY_BEGIN);
            int tryStartAddr = bytecode.size();
            emit(0);  // handler offset placeholder
            
            // Try block
            visitStatement(tryStmt->tryBlock);
            
            emit(OP_TRY_END);
            emit(OP_JMP);
            int skipCatchAddr = bytecode.size();
            emit(0);
            
            // Preenche handler offset
            bytecode[tryStartAddr] = bytecode.size();
            
            // Catch blocks
            for (auto& catchClause : tryStmt->catchClauses) {
                emit(OP_CATCH);
                visitStatement(catchClause->body);
            }
            
            bytecode[skipCatchAddr] = bytecode.size();
            
            // Finally
            if (tryStmt->finallyBlock) {
                emit(OP_FINALLY);
                visitStatement(tryStmt->finallyBlock);
            }
            break;
        }
        
        case NodeType::ThrowStmt: {
            auto throwStmt = std::static_pointer_cast<ThrowStmt>(stmt);
            visitExpression(throwStmt->exception);
            emit(OP_ATHROW);
            break;
        }
        
        case NodeType::SynchronizedStmt: {
            auto syncStmt = std::static_pointer_cast<SynchronizedStmt>(stmt);
            visitExpression(syncStmt->lockObject);
            emit(OP_DUP);
            emit(OP_MONITORENTER);
            
            // Body em try-finally para garantir unlock
            emit(OP_TRY_BEGIN);
            int tryAddr = bytecode.size();
            emit(0);
            
            visitStatement(syncStmt->body);
            
            emit(OP_TRY_END);
            emit(OP_MONITOREXIT);
            emit(OP_JMP);
            int skipAddr = bytecode.size();
            emit(0);
            
            bytecode[tryAddr] = bytecode.size();
            emit(OP_MONITOREXIT);
            emit(OP_ATHROW);  // Re-throw
            
            bytecode[skipAddr] = bytecode.size();
            break;
        }
        
        case NodeType::ExprStmt: {
            auto exprStmt = std::static_pointer_cast<ExprStmt>(stmt);
            if (exprStmt->expression) {
                visitExpression(exprStmt->expression);
                emit(OP_POP);
            }
            break;
        }
        
        case NodeType::AssertStmt: {
            auto assertStmt = std::static_pointer_cast<AssertStmt>(stmt);
            visitExpression(assertStmt->condition);
            emit(OP_JNZ);
            int okAddr = bytecode.size();
            emit(0);
            
            // Assertion failed - throw
            if (assertStmt->message) {
                visitExpression(assertStmt->message);
            }
            // TODO: throw AssertionError
            emit(OP_ATHROW);
            
            bytecode[okAddr] = bytecode.size();
            break;
        }
        
        default:
            break;
    }
}

void Codegen::visitExpression(ExprPtr expr) {
    if (!expr) return;
    
    switch (expr->getType()) {
        case NodeType::Literal: {
            auto lit = std::static_pointer_cast<LiteralExpr>(expr);
            switch (lit->litType) {
                case LiteralExpr::LitType::Null:
                    emit(OP_PUSH_NULL);
                    break;
                case LiteralExpr::LitType::Boolean:
                    emit(lit->getBoolValue() ? OP_PUSH_TRUE : OP_PUSH_FALSE);
                    break;
                case LiteralExpr::LitType::Int: {
                    int val = std::stoi(lit->value);
                    if (val >= -1 && val <= 5) {
                        emit(OP_ICONST_0 + val);
                    } else {
                        emit(OP_PUSH_INT);
                        emit(val);
                    }
                    break;
                }
                case LiteralExpr::LitType::Long:
                    emit(OP_PUSH_LONG);
                    // TODO: emit 64-bit value
                    emit(std::stoll(lit->value) & 0xFFFFFFFF);
                    emit(std::stoll(lit->value) >> 32);
                    break;
                case LiteralExpr::LitType::Float:
                    emit(OP_PUSH_FLOAT);
                    emit(*reinterpret_cast<int32_t*>(&lit->value));
                    break;
                case LiteralExpr::LitType::Double:
                    emit(OP_PUSH_DOUBLE);
                    // TODO: emit 64-bit value
                    break;
                case LiteralExpr::LitType::String:
                    emit(OP_PUSH_STRING);
                    // TODO: índice na constant pool
                    emit(999);
                    break;
                default:
                    emit(OP_PUSH_NULL);
                    break;
            }
            break;
        }
        
        case NodeType::Identifier: {
            auto id = std::static_pointer_cast<IdentifierExpr>(expr);
            if (variables.find(id->name) != variables.end()) {
                emit(OP_LOAD_GLOBAL);
                emit(variables[id->name]);
            } else {
                // Variável não encontrada - assume 0
                emit(OP_ICONST_0);
            }
            break;
        }
        
        case NodeType::BinaryExpr: {
            auto bin = std::static_pointer_cast<BinaryExpr>(expr);
            
            // Short-circuit para && e ||
            if (bin->op == BinaryExpr::Op::And) {
                visitExpression(bin->left);
                emit(OP_DUP);
                emit(OP_JZ);
                int shortCircuit = bytecode.size();
                emit(0);
                emit(OP_POP);
                visitExpression(bin->right);
                bytecode[shortCircuit] = bytecode.size();
            } else if (bin->op == BinaryExpr::Op::Or) {
                visitExpression(bin->left);
                emit(OP_DUP);
                emit(OP_JNZ);
                int shortCircuit = bytecode.size();
                emit(0);
                emit(OP_POP);
                visitExpression(bin->right);
                bytecode[shortCircuit] = bytecode.size();
            } else {
                visitExpression(bin->left);
                visitExpression(bin->right);
                
                switch (bin->op) {
                    case BinaryExpr::Op::Add: emit(OP_IADD); break;
                    case BinaryExpr::Op::Sub: emit(OP_ISUB); break;
                    case BinaryExpr::Op::Mul: emit(OP_IMUL); break;
                    case BinaryExpr::Op::Div: emit(OP_IDIV); break;
                    case BinaryExpr::Op::Mod: emit(OP_IMOD); break;
                    case BinaryExpr::Op::Eq: emit(OP_IEQ); break;
                    case BinaryExpr::Op::NotEq: emit(OP_INE); break;
                    case BinaryExpr::Op::Lt: emit(OP_ILT); break;
                    case BinaryExpr::Op::LtEq: emit(OP_ILE); break;
                    case BinaryExpr::Op::Gt: emit(OP_IGT); break;
                    case BinaryExpr::Op::GtEq: emit(OP_IGE); break;
                    case BinaryExpr::Op::BitAnd: emit(OP_IAND); break;
                    case BinaryExpr::Op::BitOr: emit(OP_IOR); break;
                    case BinaryExpr::Op::BitXor: emit(OP_IXOR); break;
                    case BinaryExpr::Op::LeftShift: emit(OP_ISHL); break;
                    case BinaryExpr::Op::RightShift: emit(OP_ISHR); break;
                    case BinaryExpr::Op::UnsignedRightShift: emit(OP_IUSHR); break;
                    default: break;
                }
            }
            break;
        }
        
        case NodeType::UnaryExpr: {
            auto unary = std::static_pointer_cast<UnaryExpr>(expr);
            
            switch (unary->op) {
                case UnaryExpr::Op::Negate:
                    visitExpression(unary->operand);
                    emit(OP_INEG);
                    break;
                case UnaryExpr::Op::Not:
                    visitExpression(unary->operand);
                    emit(OP_PUSH_INT);
                    emit(0);
                    emit(OP_IEQ);  // !x == (x == 0)
                    break;
                case UnaryExpr::Op::BitNot:
                    visitExpression(unary->operand);
                    emit(OP_PUSH_INT);
                    emit(-1);
                    emit(OP_IXOR);  // ~x == x ^ -1
                    break;
                case UnaryExpr::Op::PreInc:
                case UnaryExpr::Op::PostInc:
                    visitExpression(unary->operand);
                    emit(OP_DUP);
                    emit(OP_ICONST_1);
                    emit(OP_IADD);
                    // TODO: store back
                    break;
                case UnaryExpr::Op::PreDec:
                case UnaryExpr::Op::PostDec:
                    visitExpression(unary->operand);
                    emit(OP_DUP);
                    emit(OP_ICONST_1);
                    emit(OP_ISUB);
                    // TODO: store back
                    break;
            }
            break;
        }
        
        case NodeType::TernaryExpr: {
            auto ternary = std::static_pointer_cast<TernaryExpr>(expr);
            visitExpression(ternary->condition);
            emit(OP_JZ);
            int elseAddr = bytecode.size();
            emit(0);
            
            visitExpression(ternary->thenExpr);
            emit(OP_JMP);
            int endAddr = bytecode.size();
            emit(0);
            
            bytecode[elseAddr] = bytecode.size();
            visitExpression(ternary->elseExpr);
            
            bytecode[endAddr] = bytecode.size();
            break;
        }
        
        case NodeType::AssignExpr: {
            auto assign = std::static_pointer_cast<AssignExpr>(expr);
            visitExpression(assign->value);
            
            if (assign->target->getType() == NodeType::Identifier) {
                auto id = std::static_pointer_cast<IdentifierExpr>(assign->target);
                if (variables.find(id->name) != variables.end()) {
                    emit(OP_DUP);
                    emit(OP_STORE_GLOBAL);
                    emit(variables[id->name]);
                }
            } else if (assign->target->getType() == NodeType::MemberExpr) {
                // obj.field = value
                auto member = std::static_pointer_cast<MemberExpr>(assign->target);
                visitExpression(member->object);
                emit(OP_SWAP);
                emit(OP_PUTFIELD);
                // TODO: field index
                emit(0);
            } else if (assign->target->getType() == NodeType::ArrayAccessExpr) {
                // arr[idx] = value
                auto access = std::static_pointer_cast<ArrayAccessExpr>(assign->target);
                visitExpression(access->array);
                visitExpression(access->index);
                emit(OP_IASTORE);
            }
            break;
        }
        
        case NodeType::CompoundAssignExpr: {
            auto compound = std::static_pointer_cast<CompoundAssignExpr>(expr);
            
            // target op= value  =>  target = target op value
            if (compound->target->getType() == NodeType::Identifier) {
                auto id = std::static_pointer_cast<IdentifierExpr>(compound->target);
                if (variables.find(id->name) != variables.end()) {
                    emit(OP_LOAD_GLOBAL);
                    emit(variables[id->name]);
                    visitExpression(compound->value);
                    
                    switch (compound->op) {
                        case BinaryExpr::Op::Add: emit(OP_IADD); break;
                        case BinaryExpr::Op::Sub: emit(OP_ISUB); break;
                        case BinaryExpr::Op::Mul: emit(OP_IMUL); break;
                        case BinaryExpr::Op::Div: emit(OP_IDIV); break;
                        case BinaryExpr::Op::Mod: emit(OP_IMOD); break;
                        default: break;
                    }
                    
                    emit(OP_DUP);
                    emit(OP_STORE_GLOBAL);
                    emit(variables[id->name]);
                }
            }
            break;
        }
        
        case NodeType::MethodCallExpr: {
            auto call = std::static_pointer_cast<MethodCallExpr>(expr);
            
            // Push object (this) se não for estático
            if (call->object) {
                visitExpression(call->object);
            }
            
            // Push argumentos
            for (auto& arg : call->arguments) {
                visitExpression(arg);
            }
            
            // Invoca método
            if (call->isStaticCall) {
                emit(OP_CALL);
            } else if (call->isSuperCall) {
                emit(OP_INVOKESPEC);
            } else {
                emit(OP_INVOKE);
            }
            // TODO: method index
            emit(call->arguments.size());
            break;
        }
        
        case NodeType::NewExpr: {
            auto newExpr = std::static_pointer_cast<NewExpr>(expr);
            
            emit(OP_NEW);
            // TODO: class index
            emit(0);
            emit(OP_DUP);
            
            // Push argumentos do construtor
            for (auto& arg : newExpr->arguments) {
                visitExpression(arg);
            }
            
            emit(OP_INVOKESPEC);
            emit(newExpr->arguments.size());
            break;
        }
        
        case NodeType::NewArrayExpr: {
            auto newArr = std::static_pointer_cast<NewArrayExpr>(expr);
            
            if (newArr->dimensions.size() == 1) {
                visitExpression(newArr->dimensions[0]);
                emit(OP_NEWARRAY);
                emit(KAVA_T_INT);  // TODO: tipo correto
            } else {
                for (auto& dim : newArr->dimensions) {
                    visitExpression(dim);
                }
                emit(OP_MULTIANEW);
                emit(newArr->dimensions.size());
            }
            break;
        }
        
        case NodeType::ArrayAccessExpr: {
            auto access = std::static_pointer_cast<ArrayAccessExpr>(expr);
            visitExpression(access->array);
            visitExpression(access->index);
            emit(OP_IALOAD);
            break;
        }
        
        case NodeType::MemberExpr: {
            auto member = std::static_pointer_cast<MemberExpr>(expr);
            visitExpression(member->object);
            emit(OP_GETFIELD);
            // TODO: field index
            emit(0);
            break;
        }
        
        case NodeType::ThisExpr:
            emit(OP_ALOAD_0);  // 'this' está sempre no local 0
            break;
        
        case NodeType::SuperExpr:
            emit(OP_ALOAD_0);
            break;
        
        case NodeType::CastExpr: {
            auto cast = std::static_pointer_cast<CastExpr>(expr);
            visitExpression(cast->operand);
            emit(OP_CHECKCAST);
            // TODO: type index
            emit(0);
            break;
        }
        
        case NodeType::InstanceOfExpr: {
            auto instOf = std::static_pointer_cast<InstanceOfExpr>(expr);
            visitExpression(instOf->operand);
            emit(OP_INSTANCEOF);
            // TODO: type index
            emit(0);
            break;
        }
        
        default:
            emit(OP_PUSH_NULL);
            break;
    }
}

} // namespace Kava
