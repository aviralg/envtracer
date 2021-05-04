#include "EnvironmentAccess.h"

EnvironmentAccess* EnvironmentAccess::Which(int call_id,
                                            const std::string& fun_name,
                                            const std::string& arg_type,
                                            int which) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, fun_name, arg_type);
    env_access->set_which(which);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::N(int call_id,
                                        const std::string& fun_name,
                                        const std::string& arg_type,
                                        int n) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, fun_name, arg_type);
    env_access->set_n(n);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::XInt(int call_id,
                                           const std::string& fun_name,
                                           const std::string& arg_type,
                                           int x_int) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, fun_name, arg_type);
    env_access->set_x_int(x_int);
    return env_access;
}

EnvironmentAccess* EnvironmentAccess::XChar(int call_id,
                                            const std::string& fun_name,
                                            const std::string& arg_type,
                                            const std::string& x_char) {
    EnvironmentAccess* env_access =
        new EnvironmentAccess(call_id, fun_name, arg_type);
    env_access->set_x_char(x_char);
    return env_access;
}
