#include "interactive/condition_parser.hpp"
#include "ConditionExprLexer.h"
#include "ConditionExprParser.h"
#include "ConditionExprBaseVisitor.h"
#include <antlr4-runtime.h>
#include <any>
#include <cctype>
#include <sstream>

namespace scaffolder {

namespace {

using namespace antlr4;

// Visitor that builds ConditionData from the parse tree.
class ConditionExprToDataVisitor : public ConditionExprBaseVisitor {
public:
    antlrcpp::Any visitExpr(ConditionExprParser::ExprContext* ctx) override {
        return visit(ctx->orExpr());
    }

    antlrcpp::Any visitOrExpr(ConditionExprParser::OrExprContext* ctx) override {
        auto list = ctx->andExpr();
        if (list.empty()) return antlrcpp::Any();
        if (list.size() == 1) return visit(list[0]);
        auto c = std::make_shared<ConditionData>();
        c->or_ = std::vector<std::shared_ptr<ConditionData>>();
        for (auto* andCtx : list) {
            auto sub = std::any_cast<std::shared_ptr<ConditionData>>(visit(andCtx));
            if (sub) c->or_->push_back(sub);
        }
        return c;
    }

    antlrcpp::Any visitAndExpr(ConditionExprParser::AndExprContext* ctx) override {
        auto list = ctx->notExpr();
        if (list.empty()) return antlrcpp::Any();
        if (list.size() == 1) return visit(list[0]);
        auto c = std::make_shared<ConditionData>();
        c->and_ = std::vector<std::shared_ptr<ConditionData>>();
        for (auto* notCtx : list) {
            auto sub = std::any_cast<std::shared_ptr<ConditionData>>(visit(notCtx));
            if (sub) c->and_->push_back(sub);
        }
        return c;
    }

    antlrcpp::Any visitNotExpr(ConditionExprParser::NotExprContext* ctx) override {
        if (ctx->NOT()) {
            auto sub = std::any_cast<std::shared_ptr<ConditionData>>(visit(ctx->notExpr()));
            if (sub) {
                auto c = std::make_shared<ConditionData>();
                c->not_ = sub;
                return c;
            }
        }
        return visit(ctx->primary());
    }

    antlrcpp::Any visitPrimary(ConditionExprParser::PrimaryContext* ctx) override {
        if (ctx->DEFAULT()) {
            auto c = std::make_shared<ConditionData>();
            c->default_ = true;
            return c;
        }
        if (ctx->LPAREN() && ctx->expr()) {
            return visit(ctx->expr());
        }
        if (ctx->simple()) {
            return visit(ctx->simple());
        }
        return antlrcpp::Any();
    }

    antlrcpp::Any visitSimple(ConditionExprParser::SimpleContext* ctx) override {
        auto c = std::make_shared<ConditionData>();
        if (ctx->IDENT()) {
            c->var = ctx->IDENT()->getText();
        }
        if (ctx->op()) {
            auto* opCtx = ctx->op();
            if (opCtx->EQUALS()) c->op = "equals";
            else if (opCtx->IN()) c->op = "in";
            else if (opCtx->notIn()) c->op = "not_in";
        }
        if (ctx->value()) {
            auto* vCtx = ctx->value();
            if (vCtx->IDENT()) {
                c->value = vCtx->IDENT()->getText();
            } else if (vCtx->idList()) {
                std::vector<std::string> list;
                for (auto* id : vCtx->idList()->IDENT()) {
                    list.push_back(id->getText());
                }
                if (list.size() == 1) c->value = list[0];
                else c->value = list;
            }
        }
        return c;
    }

    antlrcpp::Any visitOp(ConditionExprParser::OpContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitNotIn(ConditionExprParser::NotInContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitValue(ConditionExprParser::ValueContext* ctx) override {
        return visitChildren(ctx);
    }

    antlrcpp::Any visitIdList(ConditionExprParser::IdListContext* ctx) override {
        return visitChildren(ctx);
    }
};

std::string to_text_impl(const std::shared_ptr<ConditionData>& c, bool in_paren) {
    if (!c) return "(none)";
    if (c->default_ && *c->default_) return "default";
    if (c->and_) {
        std::string s;
        for (size_t i = 0; i < c->and_->size(); ++i) {
            if (i) s += " AND ";
            s += to_text_impl((*c->and_)[i], true);
        }
        return in_paren ? "(" + s + ")" : s;
    }
    if (c->or_) {
        std::string s;
        for (size_t i = 0; i < c->or_->size(); ++i) {
            if (i) s += " OR ";
            s += to_text_impl((*c->or_)[i], true);
        }
        return in_paren ? "(" + s + ")" : s;
    }
    if (c->not_) return "NOT " + to_text_impl(*c->not_, true);
    std::string s = c->var ? *c->var : "?";
    if (c->op) s += " " + *c->op + " ";
    if (c->value) {
        std::visit([&s](const auto& v) {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) s += v;
            else {
                s += "(";
                for (size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + v[i];
                s += ")";
            }
        }, *c->value);
    }
    return s;
}

}  // namespace

std::shared_ptr<ConditionData> parse_condition_text(const std::string& text, std::string* out_error) {
    if (out_error) out_error->clear();
    std::string trimmed;
    for (char ch : text) {
        if (ch != '\r' && ch != '\n') trimmed += ch;
    }
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) {
        if (out_error) *out_error = "Empty condition";
        return nullptr;
    }
    size_t end = trimmed.find_last_not_of(" \t");
    trimmed = trimmed.substr(start, end - start + 1);

    try {
        ANTLRInputStream input(trimmed);
        ConditionExprLexer lexer(&input);
        CommonTokenStream tokens(&lexer);
        ConditionExprParser parser(&tokens);
        parser.removeErrorListeners();
        class CaptureErrorListener : public BaseErrorListener {
        public:
            std::string message;
            void syntaxError(Recognizer* /*r*/, Token* /*off*/, size_t line, size_t col,
                const std::string& msg, std::exception_ptr /*e*/) override {
                message = "Line " + std::to_string(line) + ":" + std::to_string(col) + " " + msg;
            }
        };
        CaptureErrorListener capture;
        parser.addErrorListener(&capture);
        ConditionExprParser::ExprContext* tree = parser.expr();
        parser.removeErrorListener(&capture);
        if (!capture.message.empty()) {
            if (out_error) *out_error = capture.message;
            return nullptr;
        }
        if (!tree) {
            if (out_error) *out_error = "Parse failed";
            return nullptr;
        }
        ConditionExprToDataVisitor visitor;
        auto result = visitor.visit(tree);
        if (!result.has_value() || result.type() != typeid(std::shared_ptr<ConditionData>)) {
            if (out_error) *out_error = "Could not build condition";
            return nullptr;
        }
        return std::any_cast<std::shared_ptr<ConditionData>>(result);
    } catch (const std::exception& e) {
        if (out_error) *out_error = e.what();
        return nullptr;
    }
}

std::string condition_to_text(const std::shared_ptr<ConditionData>& cond) {
    return to_text_impl(cond, false);
}

Condition condition_data_to_schema(const std::shared_ptr<ConditionData>& cd) {
    Condition c;
    if (!cd) return c;
    c.var = cd->var;
    c.op = cd->op;
    c.value = cd->value;
    c.default_ = cd->default_;
    if (cd->and_) {
        std::vector<std::shared_ptr<Condition>> subs;
        for (const auto& sub : *cd->and_)
            subs.push_back(std::make_shared<Condition>(condition_data_to_schema(sub)));
        c.and_ = subs;
    }
    if (cd->or_) {
        std::vector<std::shared_ptr<Condition>> subs;
        for (const auto& sub : *cd->or_)
            subs.push_back(std::make_shared<Condition>(condition_data_to_schema(sub)));
        c.or_ = subs;
    }
    if (cd->not_)
        c.not_ = std::make_shared<Condition>(condition_data_to_schema(*cd->not_));
    return c;
}

}  // namespace scaffolder
