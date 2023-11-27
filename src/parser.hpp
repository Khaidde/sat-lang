#ifndef PARSER_HPP
#define PARSER_HPP

#include "cfg.hpp"
#include "general.hpp"

namespace slang {

Result parse_to_cfg(CFG *out_cfg, cstr filepath);

} // namespace slang

#endif
