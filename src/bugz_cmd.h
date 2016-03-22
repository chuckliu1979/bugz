/* -*- mode: c; c-basic-offset: 4; -*-
 * vim: noexpandtab sw=4 ts=4 sts=0:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

_subcommand_macro_(get,    "Get bug from Bugzilla")
_subcommand_macro_(post,   "Post new bug into Bugzilla")
_subcommand_macro_(search, "Search for bugs in Bugzilla")
_subcommand_macro_(modify, "Modify a bug (e.g. post a comment)")

_subcommand_macro_(attach,     "Attach the file to a bug")
_subcommand_macro_(attachment, "Get attachment from Bugzilla")
_subcommand_macro_(history,    "Get the history for a specific bug")
_subcommand_macro_(component,  "Create new component for a specific product")

_subcommand_macro_(connections, "List known bug trackers")

_subcommand_macro_(login,       "Log into Bugzilla   (deprecated)")
_subcommand_macro_(logout,      "Log out of Bugzilla (deprecated)")

#ifdef _subcommand_macro_
#undef _subcommand_macro_
#endif

