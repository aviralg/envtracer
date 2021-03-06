#ifndef ENVTRACER_UTILITIES_H
#define ENVTRACER_UTILITIES_H

#include <vector>
#include <string>
#include "Rincludes.h"

extern const std::string ENVTRACER_NA_STRING;

extern "C" {
extern SEXP R_WhichSymbol;
extern SEXP R_NSymbol;
}

std::string get_sexp_type(SEXP r_value);

std::string get_type_as_string(SEXP r_object);

SEXP integer_vector_wrap(const std::vector<int>& vector);

SEXP real_vector_wrap(const std::vector<double>& vector);

SEXP character_vector_wrap(const std::vector<std::string>& vector);

SEXP logical_vector_wrap(const std::vector<int>& vector);

void set_class(SEXP r_object, const std::string& class_name);

SEXP create_data_frame(const std::vector<std::string>& names,
                       const std::vector<SEXP>& columns);

SEXP make_char(const std::string& input);

SEXP make_char(const std::vector<std::string>& inputs);

std::string charptr_to_string(const char* charptr);

std::string to_string(const std::vector<std::pair<std::string, int>>& seq);

std::string to_string(const std::vector<int>& seq);

#endif /* ENVTRACER_UTILITIES_H */
