#ifndef ENVTRACER_EVAL_TABLE_H
#define ENVTRACER_EVAL_TABLE_H

#include "Call.h"
#include <unordered_map>
#include "Function.h"
#include "Environment.h"
#include "Eval.h"
#include <instrumentr/instrumentr.h>

class EvalTable {
  public:
    EvalTable() {
    }

    ~EvalTable() {
        for (auto iter = table_.begin(); iter != table_.end(); ++iter) {
            delete *iter;
        }
        table_.clear();
    }

    Eval* insert(Eval* eval) {
        table_.push_back(eval);
        return eval;
    }

    SEXP to_sexp() {
        int size = table_.size();

        SEXP r_time = PROTECT(allocVector(INTSXP, size));
        SEXP r_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_direct = PROTECT(allocVector(LGLSXP, size));
        SEXP r_expression = PROTECT(allocVector(STRSXP, size));
        SEXP r_source_fun_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_backtrace = PROTECT(allocVector(STRSXP, size));

        for (int index = 0; index < table_.size(); ++index) {
            Eval* eval = table_[index];

            eval->to_sexp(index,
                          r_time,
                          r_env_id,
                          r_direct,
                          r_expression,
                          r_source_fun_id_1,
                          r_source_call_id_1,
                          r_source_fun_id_2,
                          r_source_call_id_2,
                          r_source_fun_id_3,
                          r_source_call_id_3,
                          r_source_fun_id_4,
                          r_source_call_id_4,
                          r_backtrace);
        }

        std::vector<SEXP> columns({r_time,
                                   r_env_id,
                                   r_direct,
                                   r_expression,
                                   r_source_fun_id_1,
                                   r_source_call_id_1,
                                   r_source_fun_id_2,
                                   r_source_call_id_2,
                                   r_source_fun_id_3,
                                   r_source_call_id_3,
                                   r_source_fun_id_4,
                                   r_source_call_id_4,
                                   r_backtrace});

        std::vector<std::string> names({"time",
                                        "env_id",
                                        "direct",
                                        "expression",
                                        "source_fun_id_1",
                                        "source_call_id_1",
                                        "source_fun_id_2",
                                        "source_call_id_2",
                                        "source_fun_id_3",
                                        "source_call_id_3",
                                        "source_fun_id_4",
                                        "source_call_id_4",
                                        "backtrace"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(13);

        return df;
    }

  private:
    std::vector<Eval*> table_;
};

#endif /* ENVTRACER_EVAL_TABLE_H */
