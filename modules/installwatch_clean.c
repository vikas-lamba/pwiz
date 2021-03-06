/* This program hadles file list generated by patched installwatch and
 * creates file list suitable for PWIZ. */

/* BUGS: failed/failedpath splitting can fail in multi task and multi thread environment. */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

FILE *write_file, *read_file, *failed_file, *failedpath_file, *trashed_file, *failedsave_file;

char *trash_buffer, *trash_name;
int trash_buffer_size=0, trash_buffer_max = 2048;

/* Normalizes filename inplace. */
void
normalize_filename (char *string)
{
    int last_is_slash=0;
    int ip = 0, op = 0;

    while (string[ip])
    {
	if (string[ip] == '/')
	{
	    if (last_is_slash) /* // */
		ip++;
	    else
	    {
		last_is_slash = 1;
		goto normal_slash;
	    }
	}
	else
	{
	    if (last_is_slash)
	    {
		if (string[ip] == '.')
		{
		    if (string[ip+1] == '.')
		    {
			if (string[ip+2] == '/' || string[ip+2] == 0) /* /../ */
			{
			    ip += 3;
			    /* NOTE: Works only for absolute paths. Installwatch generates only absolute paths. */
			    if (op >2) {
				op -= 2;
				while (op > 0 && string[op] != '/')
				{
				    op--;
				}
				op++;
			    }
			}
			else
			    goto normal;
		    }
		    else
		    {
			if ((string[ip+1] == '/' || string[ip+1] == 0)) /* /./ */
			    ip += 2;
			else
			    goto normal;
		    }
		}
		else
		    goto normal;
	    }
	    else
	    {
	    normal:
		last_is_slash = 0;
	    normal_slash:
		string[op++] = string[ip++];
	    }
	}
    }
    if (op > 0 && string[op-1] == '/')
	op--;
    string[op] = 0;
}

/* Read: Successfull read.
 * Failed: Unsuccessfull read.
 * Trashed: Unsuccessfull read followed by successfull read of file of the same name.
 */
void handle_read (char *filename, char *result)
{
    char *base_name;

    /* Find base name and compare it with old one. */
    base_name = basename (filename);

     /* FIXME: Error handling. */
    if (!strcmp (result, "#success"))
    {
	/* If old access was to identical file, trash the whole trash
	 * buffer, it was probably path search. */
	if (!strcmp (base_name, trash_name))
	    fwrite (trash_buffer, trash_buffer_size, 1, trashed_file);
	else
	    fwrite (trash_buffer, trash_buffer_size, 1, failedsave_file);
	trash_buffer_size = 0;
	failedsave_file = failed_file;
	fprintf (read_file, "%s\n", filename);
    }
    else
    {
	int filename_len = strlen (filename);
	if (!strcmp (base_name, trash_name))
	{
	    failedsave_file = failedpath_file;
	}
	else
	{
	    free (trash_name);
	    trash_name = strdup (base_name);
	    fwrite (trash_buffer, trash_buffer_size, 1, failedsave_file);
	    trash_buffer_size = 0;
	    failedsave_file = failed_file;
	}
	if (trash_buffer_size + filename_len + 1 + 1 > trash_buffer_max)
	{
	    trash_buffer_max *= 2;
	    trash_buffer = realloc (trash_buffer, trash_buffer_max);
	}
	sprintf (trash_buffer + trash_buffer_size, "%s\n", filename);
	trash_buffer_size += filename_len + 1;
    }
}

int main (void) {
     const char delimiters[] = "\t";

     char *tmpdir, *write_name, *read_name, *failed_name, *failedpath_name, *trashed_name, *line;
     ssize_t linesize;
     size_t bufsize = 2048;
     char *a1, *a2, *a3, *a4, *tmp;

     tmpdir = getenv("PWIZ_TMPDIR");
     if (tmpdir == NULL || tmpdir[0] == 0) {
	 tmpdir = ".";
     }

     (void) asprintf (&write_name, "%s/filelist_raw.lst", tmpdir);
     (void) asprintf (&read_name, "%s/filelist_read.lst", tmpdir);
     (void) asprintf (&failed_name, "%s/filelist_failed.lst", tmpdir);
     (void) asprintf (&failedpath_name, "%s/filelist_failedpath.lst", tmpdir);
     (void) asprintf (&trashed_name, "%s/filelist_trashed.lst", tmpdir);

     /* FIXME: Error handling. */
     write_file = fopen (write_name, "w");
     read_file = fopen (read_name, "w");
     failed_file = fopen (failed_name, "w");
     failedpath_file = fopen (failedpath_name, "w");
     trashed_file = fopen (trashed_name, "w");
     failedsave_file = failed_file;

     free (write_name);
     free (read_name);
     free (failed_name);
     free (failedpath_name);
     free (trashed_name);

     line = malloc (2048);
     trash_buffer = malloc (2048);
     trash_name = malloc (1);
     strcpy (trash_name, "");

     while ((linesize = getline (&line, &bufsize, stdin)) >= 0) {

	 line[linesize-1] = '\0';
	 tmp = line;

	 a1 = strsep (&tmp, delimiters);
	 if (tmp) {
	     a2 = strsep (&tmp, delimiters);
	     if (tmp) {
		 a3 = strsep (&tmp, delimiters);
		 if (tmp) {
		     a4 = strsep (&tmp, delimiters);
		 }
	     }
	 }

	 if (!strcmp(a2, "read"))
	 {
	     normalize_filename (a3);
	     handle_read (a3, a4);
	 }
	 else
	     if (!strcmp(a2, "open"))
	     {
		 normalize_filename (a3);
		 fprintf (write_file, "%s file\n", a3);
	     }
	     else
		 if (!strcmp(a2, "mkdir"))
		 {
		     normalize_filename (a3);
		     fprintf (write_file, "%s dir\n", a3);
		 }
		 else
		     if (!strcmp(a2, "symlink") || !strcmp(a2, "link"))
		     {
			 normalize_filename (a4);
			 fprintf (write_file, "%s file\n", a4);
		     }

     }

     fclose (write_file);
     fclose (read_file);
     fclose (failed_file);
     fclose (failedpath_file);
     fclose (trashed_file);

     exit(0);
}
