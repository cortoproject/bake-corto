/*
                                   )
                                  (.)
                                  .|.
                                  | |
                              _.--| |--._
                           .-';  ;`-'& ; `&.
                          \   &  ;    &   &_/
                           |"""---...---"""|
                           \ | | | | | | | /
                            `---.|.|.|.---'

 * This file is generated by bake.lang.c for your convenience. Headers of
 * dependencies will automatically show up in this file. Include bake_config.h
 * in your main project file. Do not edit! */

#ifndef BAKE_CORTO_BAKE_CONFIG_H
#define BAKE_CORTO_BAKE_CONFIG_H

/* Headers of public dependencies */
#include <bake.util>

/* Headers of private dependencies */
#ifdef BAKE_CORTO_IMPL
/* No dependencies */
#endif

/* Convenience macro for exporting symbols */
#if BAKE_CORTO_IMPL && defined _MSC_VER
#define BAKE_CORTO_EXPORT __declspec(dllexport)
#elif BAKE_CORTO_IMPL
#define BAKE_CORTO_EXPORT __attribute__((__visibility__("default")))
#elif defined _MSC_VER
#define BAKE_CORTO_EXPORT __declspec(dllimport)
#else
#define BAKE_CORTO_EXPORT
#endif

#endif

