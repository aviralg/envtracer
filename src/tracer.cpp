#include "tracer.h"
#include "TracingState.h"
#include "callbacks.h"
#include <instrumentr/instrumentr.h>

SEXP r_envtracer_tracer_create() {
    instrumentr_tracer_t tracer = instrumentr_tracer_create();

    instrumentr_callback_t callback;

    callback = instrumentr_callback_create_from_c_function(
        (void*) (tracing_entry_callback), INSTRUMENTR_EVENT_TRACING_ENTRY);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (tracing_exit_callback), INSTRUMENTR_EVENT_TRACING_EXIT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (package_load_callback), INSTRUMENTR_EVENT_PACKAGE_LOAD);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (package_attach_callback), INSTRUMENTR_EVENT_PACKAGE_ATTACH);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (builtin_call_exit_callback),
        INSTRUMENTR_EVENT_BUILTIN_CALL_EXIT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (closure_call_entry_callback),
        INSTRUMENTR_EVENT_CLOSURE_CALL_ENTRY);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (closure_call_exit_callback),
        INSTRUMENTR_EVENT_CLOSURE_CALL_EXIT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (promise_force_entry_callback),
        INSTRUMENTR_EVENT_PROMISE_FORCE_ENTRY);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (promise_force_exit_callback),
        INSTRUMENTR_EVENT_PROMISE_FORCE_EXIT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (promise_value_lookup_callback),
        INSTRUMENTR_EVENT_PROMISE_VALUE_LOOKUP);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (promise_substitute_callback),
        INSTRUMENTR_EVENT_PROMISE_SUBSTITUTE);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (promise_expression_lookup_callback),
        INSTRUMENTR_EVENT_PROMISE_EXPRESSION_LOOKUP);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (variable_lookup), INSTRUMENTR_EVENT_VARIABLE_LOOKUP);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (function_context_lookup),
        INSTRUMENTR_EVENT_FUNCTION_CONTEXT_LOOKUP);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (variable_assign), INSTRUMENTR_EVENT_VARIABLE_ASSIGNMENT);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (variable_define), INSTRUMENTR_EVENT_VARIABLE_DEFINITION);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (variable_remove), INSTRUMENTR_EVENT_VARIABLE_REMOVAL);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (value_finalize), INSTRUMENTR_EVENT_VALUE_FINALIZE);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (trace_error), INSTRUMENTR_EVENT_ERROR);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    callback = instrumentr_callback_create_from_c_function(
        (void*) (attribute_set_callback), INSTRUMENTR_EVENT_ATTRIBUTE_SET);
    instrumentr_tracer_set_callback(tracer, callback);
    instrumentr_object_release(callback);

    SEXP r_tracer = instrumentr_tracer_wrap(tracer);
    instrumentr_object_release(tracer);
    return r_tracer;
}
