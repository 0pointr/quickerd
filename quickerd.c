/*
* quickerd - a program to convert ERD descriptor files to graphviz input file
* Author :: debd92 [at] gmail.com
*           Copyright (C) 2015 
* Released under           :: GPL v3
* Release date (v0.1) :: 2015-04-24
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <regex.h>
#include <sys/types.h>

#ifdef __linux
    #include <unistd.h>
#elif _WIN32
    #include <windows.h>
#endif

#define MEM_CHUNK 32
#define OUTBUFF_CHUNK 1024
#define MAX_ERR_LEN 256

enum type { TB_SPEC, REL_SPEC };

struct node_t {
    char *name, *uname;
    char type;
    char to, from;  /* cardinality */
};

typedef struct node_t node;

int hndl_fatal_error(const char* func)
{
    perror(func);
    exit(1);
}

int handle_regex(char *text, const char *to_match, const int nmatch)
{
    regex_t reg_comp;
    int status = regcomp(&reg_comp, to_match, REG_EXTENDED|REG_NEWLINE);

    if (status) {
        char err_msg[MAX_ERR_LEN];
        regerror(status, &reg_comp, err_msg, MAX_ERR_LEN);
        return 0;
    }

    char *tmp = text;
    regmatch_t tmp_match[3];

    int res = regexec(&reg_comp, tmp, 2, tmp_match, 0);
    regfree(&reg_comp);
    return !res;
    /*
    int if_match=0, offset=0;
    while ( regexec(&reg_comp, tmp, 2, tmp_match, 0) == 0 ) {
        matches[if_match].rm_so = tmp_match[1].rm_so + offset;
        matches[if_match].rm_eo = tmp_match[1].rm_eo + offset;
        tmp += tmp_match[0].rm_eo;
        offset += tmp_match[0].rm_eo;
        if_match++;
    }
    matches[if_match].rm_so = matches[if_match].rm_eo = -1;
    
    regfree(&reg_comp);

    if (! if_match) { /* if no matches 
        free(matches);
        matches = NULL;
    }
    return matches;
    */
}

char *read_file_into_mem(const char *infile)
{
  FILE *fp;

  if ( (fp = fopen(infile, "r")) ) {
    fseek(fp, 0L, SEEK_END);
    long bytes = ftell(fp);
    rewind(fp);

    char *mem = malloc(bytes * sizeof(char) +1);
    if(!mem) hndl_fatal_error("malloc");

    fread(mem, 1, bytes, fp);
    fclose(fp);

    *(mem+bytes) = EOF;
    return mem;
  }
  else
    hndl_fatal_error("fopen");
}

/* get each line from memory as they appear in the file */
char *getline_from_mem(char **looper)
{
  int indx=0;
  char *temp=NULL;

  /* allocate in units of MEM_CHUNK */
  if (!(temp = malloc(MEM_CHUNK))) hndl_fatal_error("malloc");
  size_t tot_alloc = MEM_CHUNK;
  while(**looper && **looper != '\n' && isprint(**looper))
  {
    if(! ((indx+1) % MEM_CHUNK) )/* +2 to accommodate null byte */
    { temp = realloc(temp, tot_alloc+MEM_CHUNK); tot_alloc += MEM_CHUNK; }
    temp[indx++] = *((*looper)++);
  }
  if (indx == 0 && **looper != EOF) temp[indx++] = '#';
  temp[indx] = '\0';

  (*looper)++; /* point to next line */
  return temp;
}

char *split(char *str, char *delim, bool reset)
{
    static int j=0;
    if (reset) j=0;
    int len = strlen(delim);
    int i=0;
    char *tmp = str+j;
    bool one_delim = true, stop = false, got_delim = false;
    while (*tmp) {
        int k;
        /* check if any of the delimiters match with current char */
        for (k=0; k<len; k++) {
            if (*(tmp) == delim[k]) {
                got_delim = true;
                break;
            }
        }
        if (!got_delim) i++;
        /* excuse the first one delimiter and increase j to point the start from
         * the char next to the last delim. this happens each iteration
         * hence, we allow no more than one delim char to separate content/deliverables */
        else if (one_delim && j)
            { tmp++; j++; one_delim = false; got_delim = false; continue; }
        else {
            stop = true;
            break;
        }

        if (stop) break;
        tmp++;
    }
    if (!i) { j=0; return NULL; }
    char *substr = calloc(i+2, sizeof(char));
    strncpy(substr, str+j, i);
    j += i;
    return substr;
}

node **parse_content(char *mem)
{
    node **table_arr = calloc(MEM_CHUNK, sizeof(node *));
    if (!table_arr)
        hndl_fatal_error("calloc");
    
    int tot_tb_alloc = MEM_CHUNK;
    char *loop = mem;
    const char *reg_table_spec = "^[a-zA-Z0-9 _.^(]*\\([a-zA-Z0-9, _.]+\\)$";
    const char *reg_rel_spec = "^[a-zA-Z0-9 ._]+>[a-zA-Z0-9 ._]+,[a-zA-Z0-9 ._()]+,[1mnMN]{1}:[1mnMN]{1}$";
    char *work_buff = NULL;
    bool get_tbname;
    int tb_indx = 0, col_indx = 0;
    int line_no=1;

    while ( (work_buff = getline_from_mem(&loop)) && *work_buff ) {
        if (*work_buff == '#') {
            free(work_buff);
            work_buff = NULL;
            line_no++;
            continue;
        }
        int tot_col_alloc = MEM_CHUNK;
        if ( handle_regex(work_buff, reg_table_spec, 0) ) { /* parse table specs */
            get_tbname = true;
            table_arr[tb_indx] = calloc(MEM_CHUNK, sizeof(node));
            int len;
            char *tmp = NULL, *tb_name;
            char *delim = "(,)";
            while ( (tmp = split(work_buff, delim, false)) ) {
                /*
                table_arr[tb_indx][col_indx] = calloc(strlen(tmp), sizeof(char));
                strcpy(table_arr[tb_indx][col_indx], tmp);
                */
                table_arr[tb_indx][col_indx].name = tmp;
                if (get_tbname) { 
                    tb_name = tmp;
                    len = strlen(tmp);
                    get_tbname = false;
                }
                int sz = len+strlen(tmp)+7;
                table_arr[tb_indx][col_indx].uname = calloc(sz, sizeof(char));
                snprintf(table_arr[tb_indx][col_indx].uname, sz-1, "%s_%s%.3d", tb_name, tmp, col_indx);
                table_arr[tb_indx][col_indx].type = TB_SPEC;
                col_indx++;

                if (col_indx % MEM_CHUNK == 0) {
                    table_arr[tb_indx] = realloc(table_arr[tb_indx], (tot_col_alloc+MEM_CHUNK)*sizeof(node *));
                    tot_col_alloc += MEM_CHUNK;
                }
            }
        }
        else if ( handle_regex(work_buff, reg_rel_spec, 0) ) { /* parse relation specs */
            table_arr[tb_indx] = calloc(MEM_CHUNK, sizeof(node));
            char *tmp = NULL;
            char *delim = ">,";
            while ( (tmp = split(work_buff, delim, false)) ) {
                table_arr[tb_indx][col_indx].name = tmp;
                table_arr[tb_indx][col_indx].uname = NULL;
                table_arr[tb_indx][col_indx].type = REL_SPEC;
                if (col_indx == 3) {          /* cardinality */
                    char *relc = NULL;
                    relc = split(tmp, ":", true);
                    table_arr[tb_indx][col_indx].from = relc[0];
                    free(relc);
                    relc = split(tmp, ":", false);
                    table_arr[tb_indx][col_indx].to = relc[0];
                    free(relc);
                }
                col_indx++;
            }
            /* no reallocation needed since regex already verifies there can be 3 cols max*/
        }
        else {
            fprintf(stderr, "Wrong syntax in file on line: %d\n", line_no);
            free(work_buff);
            return NULL;
        }

        table_arr[tb_indx][col_indx].name = NULL;  /* since windows doesnt zero out callocs */

        tb_indx++;
        col_indx = 0;
        free(work_buff);
        work_buff = NULL;
        line_no++;

        if (tb_indx % MEM_CHUNK == 0) {
            table_arr = realloc(table_arr, (tot_tb_alloc+MEM_CHUNK)*sizeof(node *));
            tot_tb_alloc += MEM_CHUNK;
        }
    }
    table_arr[tb_indx] = NULL;
    if (work_buff) free(work_buff);

    if (!tb_indx) {
        free(table_arr);
        table_arr = NULL;
    }
    return table_arr;
}

char *get_uname(node **table, char *key)
{
    int i, j;
    for (i=0; table[i]; i++) {
        for (j=0; table[i][j].name; j++) {
            if (table[i][j].type == REL_SPEC) continue;
            if (! strcmp(table[i][j].name, key) )
                return table[i][j].uname;
        }
    }

    return NULL;
}

int sane_snprintf(char **buff, int *total, int count, char *fmt, ...)
{
    int s, r;
    va_list ap;
    va_start(ap, fmt);
    r = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    s = r;
    if (s+1 > (*total-count)) {
        s = (s+1 < OUTBUFF_CHUNK) ? OUTBUFF_CHUNK : s+1;
        *buff = realloc(*buff, *total+s);
        if (! *buff ) hndl_fatal_error("realloc");
        *total += s;
    }

    va_start(ap, fmt);
    vsnprintf((*buff)+count, s+1, fmt, ap);
    va_end(ap);

    return r;
}

void write_gv_output(node **table, char *outfile)
{
#ifndef __gui

#ifdef __linux
    if ( !access(outfile, F_OK) ) {
        char r;
        printf("File: %s exists. Overwrite? (y/n): ", outfile);
        scanf("%c", &r);
        if (r != 'y')
            return;
    }
#elif _WIN32
    DWORD dwAttrib = GetFileAttributes(szPath);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES && 
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        char r;
        printf("File: %s exists. Overwrite? (y/n): ", outfile);
        scanf("%c", &r);
        if (r != 'y')
            return;
    }
#endif

#endif
    FILE *fp = fopen(outfile, "w");
    if (!fp) {
        hndl_fatal_error("fopen");
    }
    
    const char *initial = "graph main {\n\
    ranksep=0.75;\n\
    rankdir=TB;\n\
    layout=dot;\n\
    constraint=true;\n\
    ";

    fwrite(initial, 1, strlen(initial), fp);

    char *outbuff = calloc(OUTBUFF_CHUNK, sizeof(char));
    int tot_alloc = OUTBUFF_CHUNK;
    int i, j, count=0;
    int rel_indx = 0;
    
    for (i=0; table[i]; i++) {
        if (table[i][0].type == TB_SPEC) {
            count += sane_snprintf(&outbuff, &tot_alloc, count, "\nsubgraph \"%s\" {\nnode [shape=oval]\n",
                             table[i][0].name);
            count += sane_snprintf(&outbuff, &tot_alloc, count, "\"%s\" [label=\"%s\",shape=box];\n",
                              table[i][0].uname, table[i][0].name);
            for (j=1; table[i][j].name; j++) {
                count += sane_snprintf(&outbuff, &tot_alloc, count, "\"%s\" [label=\"%s\"];\n",
                                  table[i][j].uname, table[i][j].name);

            }
            for (j=1; table[i][j].name; j++) {
                count += sane_snprintf(&outbuff, &tot_alloc, count, "\"%s\" -- \"%s\";\n",
                                  table[i][0].uname, table[i][j].uname);
            }
            count += sane_snprintf(&outbuff, &tot_alloc, count, "}\n");
            fwrite(outbuff, 1, count, fp);
            count = 0;
        }
        else if (table[i][0].type == REL_SPEC) {
            char *src = get_uname(table, table[i][0].name);
            char *dst = get_uname(table, table[i][1].name);

            if (src && dst) {
                count += sane_snprintf(&outbuff, &tot_alloc, count, "\nrel%d [label=\"%s\", shape=diamond];\n",
                                  rel_indx, table[i][2].name);
                count += sane_snprintf(&outbuff, &tot_alloc, count, "\"%s\" -- rel%d [headport=n,headlabel=%c,labeldistance=2,color=red];\n",
                                  src, rel_indx, table[i][3].from);
                count += sane_snprintf(&outbuff, &tot_alloc, count, "rel%d -- \"%s\" [tailport=s,taillabel=%c,labeldistance=2,color=red];\n",
                                  rel_indx, dst, table[i][3].to);

                fwrite(outbuff, 1, count, fp);
                count = 0;
                rel_indx++;
            }
            else { fprintf(stderr, "Unknown table in relationship %d : %s -> %s\nTable \"%s\" not defined\n",
                         rel_indx+1, table[i][0].name, table[i][1].name,
                         (src ? table[i][1].name : table[i][0].name));
                    fclose(fp);
                    free(outbuff);
                    return;
            }
        }
    }
    fputc('}', fp); /* brings closure*/

    fclose(fp);
    free(outbuff);
}

void freemem(node **table)
{
    int i, j;
    for (i=0; table[i]; i++) {
        for (j=0; table[i][j].name; j++) {
            free(table[i][j].name);
            free(table[i][j].uname);
        }
        free(table[i]);
    }
    free(table);
}

int main(int argc, char **argv)
{
    char *infile = NULL;
    char *outfile = NULL;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <table spce file> <output file>\n", argv[0]);
        fprintf(stderr, "Supply table spec file and output file names.\n");
        return 1;
    }
    else {
        infile = argv[1];
        outfile = argv[2];
    }

    char *mem = read_file_into_mem(infile);
    node **table_arr = parse_content(mem);
    free(mem);
   
    if (table_arr) {
        /*
        int i, j;
        for (i=0; table_arr[i]; i++) {
            for (j=0; table_arr[i][j].name; j++) {
                printf("%s %s\n", table_arr[i][j].name, table_arr[i][j].uname);
            }
        }
        */
        write_gv_output(table_arr, outfile);
        freemem(table_arr);
    }
    else return 1;
    
    return 0;
}
