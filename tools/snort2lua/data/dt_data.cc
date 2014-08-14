/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2002-2013 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
// dt_data.cc author Josh Rosenbaum <jrosenba@cisco.com>

#include "dt_data.h"
#include "utils/s2l_util.h"
#include <iostream>
#include <sstream>
#include <cstring>


LuaData::PrintMode LuaData::mode = LuaData::PrintMode::QUIET;

static inline Table* find_table(std::vector<Table*> vec, std::string name)
{
    if(name.empty())
        return nullptr;
    
    for( auto *t : vec)
        if(!name.compare(t->get_name()))
            return t;

    return nullptr;
}

LuaData::LuaData()
    :   curr_rule(nullptr),
        curr_rule_opt(nullptr),
        curr_data_bad(false)
{
    comments = new Comments(start_comments, 0,
                    Comments::CommentType::MULTI_LINE);
    errors = new Comments(start_errors, 0,
                    Comments::CommentType::MULTI_LINE);
    bad_rules = new Comments(start_bad_rules, 0,
                    Comments::CommentType::MULTI_LINE);
}

LuaData::~LuaData()
{
    for (auto v : vars)
        delete v;

    for (auto t : tables)
        delete t;

    for (auto r : rules)
        delete r;

    delete comments;
    delete errors;
    delete bad_rules;
}



std::string LuaData::translate_variable(std::string var_name)
{
    for (auto v : vars)
        if (!var_name.compare(v->get_name()))
            return v->get_value(this);

    return std::string();
}

/*
 * I am ashamed to say, but I have absolutely no idea what
 * Snort attempts to do when 'expanding' variables.  Since I also
 * have absolutely no inclination to figure out this mess,
 * I copied the Snort version of ExpandVars and made some
 * minor adjustments.
 *
 * Given a Snort style string to expand, this funcion will return
 * the expanded string
 */
std::string LuaData::expand_vars(std::string string)
{
    std::string estring;
    estring.resize(1024, '\0');

    char rawvarname[128], varname[128], varaux[128], varbuffer[128];
    char varmodifier;
    const char* varcontents;
    std::size_t varname_completed, i, j, iv, jv, l_string, name_only;
    char c;
    int quote_toggle = 0;

    if(string.empty() || string.rfind('$') == std::string::npos)
        return string;


    i = j = 0;
    l_string = string.size();
    std::cout << "ExpandVars, Before: " << string << std::endl;

    while(i < l_string && j < std::string::npos)
    {
        c = string[i++];

        if(c == '"')
        {
            /* added checks to make sure that we are inside a quoted string
             */
            quote_toggle ^= 1;
        }

        if(c == '$' && !quote_toggle)
        {
            std::memset((char *) rawvarname, 0, sizeof(rawvarname));
            varname_completed = 0;
            name_only = 1;
            iv = i;
            jv = 0;

            if(string[i] == '(')
            {
                name_only = 0;
                iv = i + 1;
            }

            while(!varname_completed
                  && iv < l_string
                  && jv < (int)sizeof(rawvarname) - 1)
            {
                c = string[iv++];

                if((name_only && !(isalnum(c) || c == '_'))
                   || (!name_only && c == ')'))
                {
                    varname_completed = 1;

                    if(name_only)
                        iv--;
                }
                else
                {
                    rawvarname[jv++] = (char)c;
                }
            }

            if(varname_completed || iv == l_string)
            {
                char *p;

                i = iv;

                varcontents = NULL;

                std::memset((char *) varname, 0, sizeof(varname));
                std::memset((char *) varaux, 0, sizeof(varaux));
                varmodifier = ' ';

                p = strchr(rawvarname, ':');
                if (p)
                {
                    std::strncpy(varname, rawvarname, (std::size_t)(p - rawvarname));

                    if(strlen(p) >= 2)
                    {
                        varmodifier = *(p + 1);
                        std::strncpy(varaux, p + 2, sizeof(varaux));
                    }
                }
                else
                    std::strncpy(varname, rawvarname, sizeof(varname));

                std::memset((char *) varbuffer, 0, sizeof(varbuffer));

                std::string tmp = translate_variable(varname);
                varcontents = tmp.c_str();

                switch(varmodifier)
                {
                    case '-':
                        if(!varcontents || !strlen(varcontents))
                            varcontents = varaux;
                        break;

                    case '?':
                        if(!varcontents || !strlen(varcontents))
                            return std::string();
                        break;
                }

                /* If variable not defined now, we're toast */
                if(!varcontents || !strlen(varcontents))
                {
                    return std::string();
                }

                if(varcontents)
                {
                    std::size_t l_varcontents = strlen(varcontents);

                    iv = 0;

                    if (estring.size() < j + l_varcontents)
                        estring.resize(estring.size() * 2);

                    while(iv < l_varcontents && j < estring.size() - 1)
                        estring[j++] = varcontents[iv++];
                }
            }
            else
            {
                if (estring.size() < j+ 1)
                    estring.resize(estring.size() * 2, '\0');

                estring[j++] = '$';
            }
        }
        else
        {
            if (estring.size() < j+ 1)
                estring.resize(estring.size() * 2, '\0');

            estring[j++] = (char)c;
        }
    }

    if (estring.size() < j)
        estring.resize(estring.size() + 1, '\0');
    else
        estring.resize(j);

    estring[j] = '\0';
    std::cout << "ExpandVars, After: " << estring << std::endl;

    return estring;
}


void LuaData::failed_conversion(std::istringstream& stream)
{
    // we only need to go through this once.
    if (!curr_data_bad)
    {
        errors->add_text(std::string());
        errors->add_text("Failed to entire convert the following line:");
        errors->add_text(stream.str());
        curr_data_bad = true;
    }
}

void LuaData::failed_conversion(std::istringstream& stream,
                                std::string unkown_option)
{
    // we only need to go through this once.
    if (!curr_data_bad)
    {
        errors->add_text(std::string());
        errors->add_text("Failed to entire convert the following line:");
        errors->add_text(stream.str());
        curr_data_bad = true;
    }
    errors->add_text("^^^^ unkown_option=" + unkown_option);
}


bool LuaData::add_variable(std::string name, std::string value)
{
    for (auto v : vars)
        if (!name.compare(v->get_name()))
            return v->add_value(value);

    Variable *var = new Variable(name);
    vars.push_back(var);
    return var->add_value(value);
}


void LuaData::reset_state()
{
    std::stack<Table*> empty;
    open_tables.swap(empty);
    curr_rule = nullptr;
    curr_rule_opt = nullptr;
    curr_data_bad = false;
}

bool LuaData::add_include_file(std::string file_name)
{

    Include* incl = new Include(file_name);

    if (incl == nullptr)
        return false;

    includes.push_back(incl);
    return true;
}

void LuaData::open_top_level_table(std::string table_name)
{
    Table *t = find_table(tables, table_name);

    if (t == nullptr)
    {
        t = new Table(table_name, 0);
        tables.push_back(t);
    }

    open_tables.push(t);
}

void LuaData::open_table(std::string table_name)
{
    Table *t;

    // if no open tables, create a top-level table
    if (open_tables.size() > 0)
    {
        t = open_tables.top()->open_table(table_name);
    }
    else
    {
        t = find_table(tables, table_name);

        if (t == nullptr)
        {
            t = new Table(table_name, 0);
            tables.push_back(t);
        }
    }

    open_tables.push(t);
}

void LuaData::open_table()
{
    // if no open tables, create a top-level table
    if (open_tables.size() == 0)
    {
        developer_error("A nameless table must be nested inside a table!!");
    }
    else
    {
        Table *t = open_tables.top()->open_table();
        open_tables.push(t);
    }
}

void LuaData::close_table()
{
    if (open_tables.size() == 0)
        developer_error("No open tables to close!!");
    else
        open_tables.pop();
}


void LuaData::developer_error(std::string error_string)
{
    std::cout << error_string << std::endl;
}

void LuaData::add_comment(std::string c)
{
    comments->add_text(c);
}


bool LuaData::add_option_to_table(const std::string option_name, const std::string val)
{
    if(open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: " +
            option_name + " = " + val);
        return false;
    }

    Table *t = open_tables.top();
    t->add_option(option_name, val);
    return true;
}

bool LuaData::add_option_to_table(const std::string option_name, const int val)
{
    if(open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: " +
            option_name + " = " + std::to_string(val));
        return false;
    }

    Table *t = open_tables.top();
    t->add_option(option_name, val);
    return true;
}

bool LuaData::add_option_to_table(const std::string option_name, const bool val)
{
    if(open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: " +
            option_name + " = " + std::to_string(val));
        return false;
    }

    Table *t = open_tables.top();
    t->add_option(option_name, val);
    return true;
}

// compilers are fickle and dangerous creatures.  Ensure a literal gets
// sent here rather to become a bool
bool LuaData::add_option_to_table(const std::string name, const char* v)
{
    return add_option_to_table(name, std::string(v));
}

bool LuaData::add_list_to_table(std::string list_name, std::string next_elem)
{
    if(open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: " +
            list_name + " = " + next_elem);
        return false;
    }

    Table *t = open_tables.top();

    if(t)
    {
        t->add_list(list_name, next_elem);
        return true;
    }
    else
    {
        developer_error("Must open table before adding an list!!: " +
            list_name + " += " + next_elem);
        return false;
    }
}

bool LuaData::add_comment_to_table(std::string comment)
{
    if (open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: '" +
            comment + "'");
        return false;
    }

    open_tables.top()->add_comment(comment);
    return true;
}

bool LuaData::add_diff_option_comment(std::string orig_var, std::string new_var)
{
    std::string error_string = "option change: '" + orig_var + "' --> '"
            + new_var + "'";

    if (open_tables.size() == 0)
    {
        developer_error("Must open table before adding an option!!: " +
            orig_var + " = " + new_var);
        return false;
    }

    open_tables.top()->add_comment(error_string);
    return true;
}


bool LuaData::add_deleted_comment(std::string dep_var)
{
    std::string error_string = "option deleted: '" + dep_var + "'";

    if (open_tables.size() == 0)
    {
        developer_error("Must open a table before adding "
            "deprecated comment!!: " + dep_var);
        return false;
    }

    open_tables.top()->add_comment(error_string);
    return true;
}

bool LuaData::add_unsupported_comment(std::string unsupported_var)
{
    std::string unsupported_str = "option '" + unsupported_var +
        "' is current unsupported";

    if (open_tables.size() == 0)
    {
        developer_error("Must open a tablebefore adding an "
            "'unsupported' comment");
        return false;
    }

    open_tables.top()->add_comment(unsupported_str);
    return true;
}

// RULE PARSING

void LuaData::begin_rule()
{
    if (curr_rule == nullptr)
    {
        curr_rule = new Rule();
        rules.push_back(curr_rule);
        curr_data_bad = false;
    }
}

void LuaData::make_rule_a_comment()
{
    if (!curr_rule)
        begin_rule();

    curr_rule->make_comment();
}

void LuaData::bad_rule(std::istringstream& stream, std::string bad_option)
{
    if (!curr_rule)
        begin_rule();

    // we only need to go through this once.
    if (!curr_data_bad)
    {
        bad_rules->add_text(std::string());
        bad_rules->add_text("Failed to convert rule: " + stream.str() + ")");
        curr_rule->bad_rule();
        curr_data_bad = true;
    }
    bad_rules->add_text("^^^^ unkown_option=" + bad_option);
}

bool LuaData::add_hdr_data(std::string data)
{

    if (!curr_rule)
        begin_rule();

    return curr_rule->add_hdr_data(data);
}

bool LuaData::add_rule_option(std::string opt_name)
{
    if (!curr_rule)
        begin_rule();

    return curr_rule->add_option(opt_name);
}

bool LuaData::add_rule_option(std::string opt_name, std::string val)
{
    if (!curr_rule)
        begin_rule();

    return curr_rule->add_option(opt_name, val);
}


bool LuaData::add_rule_option_before_selected(std::string keyword,
                                            std::string val)
{

    if (!curr_rule_opt)
    {
        comments->add_text("Select an option before placing a "
                "new option before selected option");
        return false;
    }

    return curr_rule->add_option_before_selected(curr_rule_opt, keyword, val);
}

bool LuaData::add_suboption(std::string keyword)
{
    if (curr_rule_opt)
        return curr_rule_opt->add_suboption(keyword);

    developer_error("Select an option before adding a suboption!!");
    return false;
}

bool LuaData::add_suboption(std::string keyword,
                            std::string val)
{
    if (curr_rule_opt)
        return curr_rule_opt->add_suboption(keyword, val);

    developer_error("Select an option before adding a suboption!!");
    return false;
}

bool LuaData::select_option(std::string opt_name)
{
    // using add_comment here so this error is right above the failed rule
    if (curr_rule)
    {
        curr_rule_opt = curr_rule->select_option(opt_name);
        if (curr_rule_opt != nullptr)
            return true;
        else
            comments->add_text("Option " + opt_name + "never created for following rule:");
    }
    else
    {
        comments->add_text("Must begin a rule before selecting an option!");
    }

    return false;
}

void LuaData::unselect_option()
{
    curr_rule_opt = nullptr;
}

void LuaData::add_comment_to_rule(std::string comment)
{
    if (!curr_rule)
        begin_rule();

    curr_rule->add_comment(comment);
}

std::ostream& operator<<( std::ostream &out, const LuaData& data)
{
    if (data.mode == LuaData::PrintMode::DEFAULT)
    {
        if (!data.errors->empty())
            out << (*data.errors) << "\n";

        if (!data.bad_rules->empty())
            out << (*data.bad_rules) << "\n";
    }

    for (Include* i : data.includes)
        out << *i << "\n";
    out << "\n\n";

    for (Variable* v : data.vars)
        out << (*v) << "\n\n";
    out << "\n\n";

    out << "default_rules =\n[[\n";
    for (Rule* r : data.rules)
        out << (*r) << "\n\n";

    out << "]]\n\n\n";


    for (Table* t : data.tables)
        out << (*t) << "\n\n";
    out << "\n\n";

    if (data.mode == LuaData::PrintMode::DEFAULT &&
        !data.comments->empty())
        out << (*data.comments) << "\n" << std::endl;

    return out;
}


void LuaData::print_rules(std::ostream& out, bool in_rule_file)
{
    if (!in_rule_file)
        out << "default_rules =\n[[\n";

    for (Rule* r : rules)
        out << (*r) << "\n";

    if (!in_rule_file)
        out << "]]\n";
}

void LuaData::print_rejects(std::ostream& out)
{

    if (mode == LuaData::PrintMode::DEFAULT)
    {
        if (!errors->empty())
            out << (*errors) << "\n";

        if (!bad_rules->empty())
            out << (*bad_rules) << "\n\n";
    }
}

void LuaData::print_conf_options(std::ostream& out)
{
    for (Variable* v : vars)
        out << (*v) << "\n\n";

    for (Table* t : tables)
        out << (*t) << "\n\n";

    if (mode == LuaData::PrintMode::DEFAULT &&
        !comments->empty())
        out << (*comments) << "\n";
}


void LuaData::swap_rules(std::vector<Rule*>& new_rules)
{
    rules.swap(new_rules);
}

void LuaData::swap_conf_data(std::vector<Variable*>& new_vars,
                                std::vector<Table*>& new_tables,
                                std::vector<Include*>& new_includes,
                                Comments*& new_comments)
{
    vars.swap(new_vars);
    tables.swap(new_tables);
    includes.swap(new_includes);

    Comments* tmp = new_comments;
    new_comments = comments;
    comments = tmp;
}
