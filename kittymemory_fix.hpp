#pragma once

#ifndef KITTYMEMORY_FIX_HPP
#define KITTYMEMORY_FIX_HPP

#include <functional>
#include <string>
#include <cstring>
#include <cerrno>

#if defined(__linux__) && !defined(__ANDROID__)
#include <string.h>
#endif

#endif // KITTYMEMORY_FIX_HPP