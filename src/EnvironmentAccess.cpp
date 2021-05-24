#include "EnvironmentAccess.h"

EnvironmentAccess* EnvironmentAccess::Which(int call_id,
                                            int depth,
                                            const std::string& fun_name,
                                            const std::string& arg_type,
                                            int which,
                                            int env_id) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_which(which);
    env_access->set_env_id(env_id);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::N(int call_id,
                                        int depth,
                                        const std::string& fun_name,
                                        const std::string& arg_type,
                                        int n,
                                        int env_id) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_n(n);
    env_access->set_env_id(env_id);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::XInt(int call_id,
                                           int depth,
                                           const std::string& fun_name,
                                           const std::string& arg_type,
                                           int x_int) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_x_int(x_int);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::XChar(int call_id,
                                            int depth,
                                            const std::string& fun_name,
                                            const std::string& arg_type,
                                            const std::string& x_char) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_x_char(x_char);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::Fun(int call_id,
                                          int depth,
                                          const std::string& fun_name,
                                          const std::string& arg_type,
                                          int fun_id) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_fun_id(fun_id);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::Lock(int call_id,
                                           int depth,
                                           const std::string& fun_name,
                                           const std::string& arg_type,
                                           const std::string& symbol,
                                           int bindings,
                                           int env_id) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, depth, fun_name, arg_type);
    env_access->set_symbol(symbol);
    env_access->set_bindings(bindings);
    env_access->set_env_id(env_id);
    return env_access;
}
