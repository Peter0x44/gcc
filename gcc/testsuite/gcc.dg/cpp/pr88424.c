/* { dg-do preprocess } */
/* { dg-options "-C" } */

/* Test that CRLF line endings in comments are preserved correctly with -C.
   Previously, the preprocessor would convert CRLF sequences in comments
   to LFLF, causing extra blank lines in the output. 
   
   This test ensures that comments with CRLF line endings maintain their
   original number of newlines when processed with the -C option.  */

/* This comment has CRLF line endings
 and should appear as two lines in the output, not three.  */

/* Check that the comment does appear correctly with single line feed. */
/* { dg-final { scan-file pr88424.i "comment has CRLF line endings\n and should appear" } } */
