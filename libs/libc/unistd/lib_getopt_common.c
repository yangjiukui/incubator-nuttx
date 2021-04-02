/****************************************************************************
 * libs/libc/unistd/lib_getopt_common.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <string.h>

#include "unistd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef optarg
#undef opterr
#undef optind
#undef optopt

/****************************************************************************
 * Prive Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getopt_long_option
 *
 * Description:
 *   Handle one long option
 *
 ****************************************************************************/

static int getopt_long_option(FAR struct getopt_s *go,
                              FAR char * const argv[],
                              FAR const struct option *longopts,
                              FAR int *longindex)
{
  int ndx;
  int ret;

  /* The option list may not be NULL in this context */

  if (longopts == NULL)
    {
      goto errout;
    }

  /* Search the list of long options for a matching name.
   * The last element of the option arry must be filled with zeroes.
   */

  for (ndx = 0; longopts[ndx].name[0] != '\0'; ndx++)
    {
      if (strcmp(go->go_optptr, longopts[ndx].name) == 0)
        {
          /* Found the option with the matching name.  Does it have an
           * required argument?  And optional argument?
           */

          switch (longopts[ndx].has_arg)
            {
              FAR char *next;

              case no_argument:
                /* No, no arguments. Just return the option that we
                 * found.
                 */

                go->go_optptr = NULL;
                go->go_optind++;
                break;

              case optional_argument:
                /* Check if there is a following argument and if that
                 * following argument is another option.
                 */

                next = argv[go->go_optind + 1];
                if (next == NULL || next[0] == '-')
                  {
                    go->go_optptr = NULL;
                    go->go_optind++;
                    break;
                  }

                 /* Fall through and treat as a required option */

              case required_argument:

                /* Verify that the required option is present */

                next = argv[go->go_optind + 1];
                if (next == NULL || next[0] == '-')
                  {
                    go->go_optptr = NULL;
                    go->go_optarg = NULL;
                    go->go_optind++;
                    return '?';
                  }

                /* Return the required option */

                go->go_optptr  = NULL;
                go->go_optarg  = next;
                go->go_optind += 2;
                break;

              default:
                goto errout;
                break;
            }

          /* Setup return value.
           *
           * 'val' is the value to return on success, or to load into the
           * variable pointed to by flag.
           *
           * 'flag' specifies how results are returned for a long option. If
           * flag is NULL, then getopt_long() returns val.  Otherwise,
           * getopt_long() returns 0, and flag points to a variable which is
           * set to val if the option is found, but left unchanged if the
           * option is not found.
           */

          if (longopts[ndx].flag != NULL)
            {
              *(longopts[ndx].flag) = longopts[ndx].val;
              ret = OK;
            }
          else
            {
              ret = longopts[ndx].val;
            }

          /* If longindex is not NULL, it points to a variable which is
           * set to the index of the long option relative to longopts.
           */

          if (longindex != NULL)
            {
              *longindex = ndx;
            }

          return ret;
        }
    }

  /* This option is not in the list of valid options */

  go->go_optopt = *go->go_optptr;
  go->go_optptr++;
  return '?';

errout:

  /* Restore the initial, uninitialized state */

  go->go_binitialized = false;
  return ERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: getopt_common
 *
 * Description:
 *
 *   getopt_common() is the common, internal implementation of getopt(),
 *   getopt_long(), and getopt_long_only().
 *
 *   getopt() parses command-line arguments.  Its arguments argc and argv
 *   are the argument count and array as passed to the main() function on
 *   program invocation.  An element of argv that starts with '-' is an
 *   option element. The characters of this element (aside from the initial
 *   '-') are option characters. If getopt() is called repeatedly, it
 *   returns successively each of the option characters from each of the
 *   option elements.
 *
 *   If getopt() finds another option character, it returns that character,
 *   updating the external variable optind and a static variable nextchar so
 *   that the next call to getopt() can resume the scan with the following
 *   option character or argv-element.
 *
 *   If there are no more option characters, getopt() returns -1. Then optind
 *   is the index in argv of the first argv-element that is not an option.
 *
 *   The 'optstring' argument is a string containing the legitimate option
 *   characters. If such a character is followed by a colon, this indicates
 *   that the option requires an argument.  If an argument is required for an
 *   option so getopt() places a pointer to the following text in the same
 *   argv-element, or the text of the following argv-element, in optarg.
 *
 *   The getopt_long() function works like getopt() except that it also
 *   accepts long options, started with two dashes. (If the program accepts
 *   only long options, then optstring should be specified as an empty
 *   string (""), not NULL.) Long option names may be abbreviated if the
 *   abbreviation is unique or is an exact match for some defined option
 *
 *   getopt_long_only() is like getopt_long(), but '-' as well as "--" can
 *   indicate a long option. If an option that starts with '-' (not "--")
 *   doesn't match a long option, but does match a short option, it is
 *   parsed as a short option instead.
 *
 *   NOTES:
 *   1. opterr is not supported and this implementation of getopt() never
 *      printfs error messages.
 *   2. getopt is NOT threadsafe!
 *   3. This version of getopt() does not reset global variables until
 *      -1 is returned.  As a result, your command line parsing loops
 *      must call getopt() repeatedly and continue to parse if other
 *      errors are returned ('?' or ':') until getopt() finally returns -1.
 *     (You can also set optind to -1 to force a reset).
 *
 * Returned Value:
 *   If an option was successfully found, then getopt() returns the option
 *   character. If all command-line options have been parsed, then getopt()
 *   returns -1.  If getopt() encounters an option character that was not
 *   in optstring, then '?' is returned. If getopt() encounters an option
 *   with a missing argument, then the return value depends on the first
 *   character in optstring: if it is ':', then ':' is returned; otherwise
 *   '?' is returned.
 *
 ****************************************************************************/

int getopt_common(int argc, FAR char * const argv[],
                  FAR const char *optstring,
                  FAR const struct option *longopts,
                  FAR int *longindex,
                  enum getopt_mode_e mode)
{
  /* Get thread-specific getopt() variables */

  FAR struct getopt_s *go = getoptvars();
  if (go == NULL)
    {
      return '?';
    }

  /* Verify input parameters. */

  if (argv != NULL)
    {
      FAR char *optchar;
      int noarg_ret = '?';

      /* The initial value of optind is 1.  If getopt() is called again in
       * the program, optind must be reset to some value <= 1.
       */

      if (go->go_optind < 1 || !go->go_binitialized)
        {
          go->go_optarg       = NULL;
          go->go_optind       = 1;     /* Skip over the program name */
          go->go_optopt       = '?';
          go->go_optptr       = NULL;  /* Start at the beginning of the first argument */
          go->go_binitialized = true;  /* Now we are initialized */
        }

      /* If the first character of opstring s ':', then ':' is in the event
       * of a missing argument. Otherwise '?' is returned.
       */

      if (*optstring == ':')
        {
           noarg_ret = ':';
           optstring++;
        }

      /* Are we resuming in the middle, or at the end of a string of
       * arguments? optptr == NULL means that we are started at the
       * beginning of argv[optind]; *optptr == \0 means that we are
       * starting at the beginning of optind+1
       */

      while (!go->go_optptr || !*go->go_optptr)
        {
          /* We need to start at the beginning of the next argv. Check if we
           * need to increment optind
           */

          if (go->go_optptr)
            {
              /* Yes.. Increment it and check for the case where where we
               * have processed everything in the argv[] array.
               */

              go->go_optind++;
            }

          /* Check for the end of the argument list */

          go->go_optptr = argv[go->go_optind];
          if (!go->go_optptr)
            {
              /* There are no more arguments, we are finished */

              go->go_binitialized = false;
              return ERROR;
            }

          /* We are starting at the beginning of argv[optind].  In this case,
           * the first character must be '-'
           */

          if (*go->go_optptr != '-')
            {
              /* The argument does not start with '-', we are finished */

              go->go_binitialized = false;
              return ERROR;
            }

          /* Skip over the '-' */

          go->go_optptr++;
        }

      /* Special case handling of "-" and "-:" */

      if (!*go->go_optptr)
        {
          /* We'll fix up optptr the next time we are called */

          go->go_optopt = '\0';
          return '?';
        }

      /* Handle the case of "-:" */

      if (*go->go_optptr == ':')
        {
          go->go_optopt = ':';
          go->go_optptr++;
          return '?';
        }

      /* go->go_optptr now points at the next option and it is not something
       * crazy.  Possibilities:
       *
       *  -o
       *  -o reqarg
       *  -option
       *  -option reqarg
       *  -option optarg
       *  --option reqarg
       *  --option optarg
       *
       * Where:
       *   o      - Some short option
       *   option - Some long option
       *   reqarg - A required argument
       *   optarg - An optional argument
       */

      /* Check for --option forms or -option forms */

      if (GETOPT_HAVE_LONG(mode))
        {
          /* Handle -option and --option forms. */

          if (*go->go_optptr == '-')
            {
              /* Skip over the second '-' */

              go->go_optptr++;

              /* And parse the long option */

              return getopt_long_option(go, argv, longopts, longindex);
            }

          /* The -option form is only valid in getop_long_only() mode and
           * must be distinguished from the -o case forms.
           */

          if (GETOPT_HAVE_LONGONLY(mode) && *(go->go_optptr + 1) != '\0')
            {
              return getopt_long_option(go, argv, longopts, longindex);
            }
        }

      /* Check if the option is in the list of valid short options.
       * In long option modes, opstring may be NULL. However, but that is
       * an error in any case here because we have not found any
       * long options.
       */

      if (optstring == NULL)
        {
          goto errout;
        }

      optchar = strchr(optstring, *go->go_optptr);
      if (!optchar)
        {
          /* No this character is not in the list of valid options */

          go->go_optopt = *go->go_optptr;
          go->go_optptr++;
          return '?';
        }

      /* Yes, the character is in the list of valid options.  Does it have an
       * required argument?
       */

      if (optchar[1] != ':')
        {
          /* No, no arguments. Just return the character that we found */

          go->go_optptr++;
          return *optchar;
        }

      /* Yes, it has a required argument.  Is the required argument
       * immediately after the command in this same argument?
       */

      if (go->go_optptr[1] != '\0')
        {
          /* Yes, return a pointer into the current argument */

          go->go_optarg = &go->go_optptr[1];
          go->go_optind++;
          go->go_optptr = NULL;
          return *optchar;
        }

      /* No.. is the optional argument the next argument in argv[] ? */

      if (argv[go->go_optind + 1] && *argv[go->go_optind + 1] != '-')
        {
          /* Yes.. return that */

          go->go_optarg = argv[go->go_optind + 1];
          go->go_optind += 2;
          go->go_optptr = NULL;
          return *optchar;
        }

      /* No argument was supplied */

      go->go_optptr = NULL;
      go->go_optarg = NULL;
      go->go_optopt = *optchar;
      go->go_optind++;
      return noarg_ret;
    }

errout:

  /* Restore the initial, uninitialized state */

  go->go_binitialized = false;
  return ERROR;
}