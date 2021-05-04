#include <R.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL
#include "tracer.h"
#include <instrumentr/instrumentr.h>
#include "utilities.h"

INSTRUMENTR_DEFINE_API()

extern "C" {

SEXP R_WhichSymbol;
SEXP R_NSymbol;

static const R_CallMethodDef callMethods[] = {
    {"envtracer_tracer_create", (DL_FUNC) &r_envtracer_tracer_create, 0},
    {NULL, NULL, 0}};

void R_init_envtracer(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);

    R_WhichSymbol = Rf_install("which");
    R_NSymbol = Rf_install("n");

    INSTRUMENTR_INITIALIZE_API()
}
}
