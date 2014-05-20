package CParse;

use strict;
use warnings;

our @keys
  = (
     'typedef',
     'struct',
     'enum',
     'union',
     'extern',
     'void',
     '__extension__',
     '__attribute__((deprecated))',
     'int',
     'long',
    );
our %key;
@key{@keys} = ();

our @keys2
  = (
     'BU_RB_WALK_ORDER',
     'ClientData',
     'FILE',
     'TclPlatStubs',
     'TclStubs',
     'Tcl_AsyncHandler',
     'Tcl_Channel',
     'Tcl_ChannelType',
     'Tcl_ChannelTypeVersion',
     'Tcl_Command',
     'Tcl_DecrRefCount',
     'Tcl_DriverBlockModeProc',
     'Tcl_DriverClose2Proc',
     'Tcl_DriverCloseProc',
     'Tcl_DriverFlushProc',
     'Tcl_DriverGetHandleProc',
     'Tcl_DriverGetOptionProc',
     'Tcl_DriverHandlerProc',
     'Tcl_DriverInputProc',
     'Tcl_DriverOutputProc',
     'Tcl_DriverSeekProc',
     'Tcl_DriverSetOptionProc',
     'Tcl_DriverThreadActionProc',
     'Tcl_DriverTruncateProc',
     'Tcl_DriverWatchProc',
     'Tcl_DriverWideSeekProc',
     'Tcl_Encoding',
     'Tcl_Event',
     'Tcl_ExitProc',
     'Tcl_Filesystem',
     'Tcl_HashEntry',
     'Tcl_HashKeyType',
     'Tcl_HashTable',
     'Tcl_IncrRefCount',
     'Tcl_Interp',
     'Tcl_InterpState',
     'Tcl_IsShared',
     'Tcl_Mutex',
     'Tcl_Namespace',
     'Tcl_Obj',
     'Tcl_ObjType',
     'Tcl_PathType',
     'Tcl_PathType',
     'Tcl_Pid',
     'Tcl_RegExp',
     'Tcl_StatBuf',
     'Tcl_ThreadId',
     'Tcl_TimerToken',
     'Tcl_Trace',
     'Tcl_UniChar',
     'Tcl_WideInt',
     '_G_fpos_t',
     '_IO_FILE',
     '_IO_FILE_plus;',
     '_IO_jump_t;',
     '_IO_marker',
     '__blkcnt_t',
     '__blksize_t',
     '__builtin_va_list',
     '__caddr_t',
     '__clock_t',
     '__clockid_t',
     '__codecvt_result',
     '__const',
     '__daddr_t',
     '__dev_t',
     '__fd_mask',
     '__fsblkcnt_t',
     '__fsfilcnt_t',
     '__fsid_t',
     '__gid_t',
     '__gnuc_va_list',
     '__id_t',
     '__ino_t',
     '__jmp_buf_tag',
     '__key_t',
     '__loff_t',
     '__mode_t',
     '__nlink_t',
     '__off64_t',
     '__off_t',
     '__pid_t',
     '__quad_t',
     '__sigset_t',
     '__ssize_t',
     '__suseconds_t',
     '__time_t',
     '__timer_t',
     '__u_char',
     '__u_int',
     '__u_long',
     '__u_quad_t',
     '__u_short',
     '__uid_t',
     'bu_attribute_value_pair',
     'bu_attribute_value_set',
     'bu_bitv',
     'bu_color',
     'bu_endian_t',
     'bu_external',
     'bu_hash_entry',
     'bu_hash_record',
     'bu_hash_tbl',
     'bu_heap_func_t',
     'bu_hist',
     'bu_hook_list',
     'bu_lex_key',
     'bu_lex_t_dbl',
     'bu_lex_t_id',
     'bu_lex_t_int',
     'bu_lex_t_key',
     'bu_lex_token',
     'bu_list',
     'bu_mapped_file',
     'bu_observer',
     'bu_ptbl',
     'bu_rb_list',
     'bu_rb_node',
     'bu_rb_package',
     'bu_rb_tree',
     'bu_structparse',
     'bu_utctime(struct',
     'bu_vlb',
     'bu_vls',
     'char',
     'const',
     'div_t',
     'double',
     'drand48_data',
     'enum',
     'extern',
     'float',
     'genptr_t',
     'int',
     'int64_t',
     'jmp_buf',
     'ldiv_t',
     'long',
     'off_t',
     'random_data',
     'short',
     'signed',
     'size_t',
     'struct',
     'timespec',
     'timeval',
     'typedef',
     'union',
     'unsigned',
     'utimbuf;',
     'void',
     'volatile',
     'wait',
    );
our %key2;
@key2{@keys2} = ();

sub extract_object {

  my $lines_aref = shift @_; # \@lines
  my $i          = shift @_; # $i - current @lines index
  my $fp         = shift @_; # ouput file pointer

  my $nl = scalar @{$lines_aref};

  # we're at the first line of the unknown object
  # get all lines til the last
  my @olines = ();

  # track '{}', '()', '[]' levels
  my $clevel = 0; # curly braces
  my $plevel = 0; # parentheses
  my $slevel = 0; # square brackets

  my $first_index = $i;
  my $last_index;

 LINE:
  for (; $i < $nl; ++$i) {
    my $lnum = $i + 1;
    my $line = $lines_aref->[$i];
    my $len = length $line;
    for (my $j = 0; $j < $len; ++$j) {
      my $c = substr $line, $j, 1;
      if ($c eq '{') {
	++$clevel;
      }
      elsif ($c eq '}') {
	--$clevel;
      }

      elsif ($c eq '(') {
	++$plevel;
      }
      elsif ($c eq ')') {
	--$plevel;
      }

      elsif ($c eq '[') {
	++$slevel;
      }
      elsif ($c eq ']') {
	--$slevel;
      }

      elsif ($c eq ';'
	     && $clevel == 0
	     && $plevel == 0
	     && $slevel == 0
	    ) {
	push @olines, $line;
	$last_index = $i;
	last LINE;
      }
    }
    push @olines, $line;
  }

  # print good lines to output file
  print  $fp "\n";
  printf $fp "=== starting extracted code at input line %d:\n", $first_index + 1;
  print  $fp "$_" for @olines;
  printf $fp "=== ending extracted code at input line %d:\n", $last_index + 1;
  print  $fp "\n";

  return $last_index;

} # extract_object

=pod

sub extract_enum {
  # example:
  #   typedef enum {
  #       BU_LITTLE_ENDIAN = 1234,
  #       BU_BIG_ENDIAN = 4321,
  #       BU_PDP_ENDIAN = 3412
  #   } bu_endian_t;

  my $lines_aref = shift @_; # \@lines
  my $i          = shift @_; # $i - current @lines index
  my $fp         = shift @_; # ouput file pointer

  my $nl = scalar @{$lines_aref};

  # we're at the first line of the enum
  # get all lines til the last
  my @olines = ();
  # track '{}' levels
  my $level = 0;
  my $first_index = $i;
  my $last_index;

 LINE:
  for (; $i < $nl; ++$i) {
    my $lnum = $i + 1;
    my $line = $lines_aref->[$i];
    my $len = length $line;
    for (my $j = 0; $j < $len; ++$j) {
      my $c = substr $line, $j, 1;
      if ($c eq '{') {
	++$level;
      }
      elsif ($c eq '}') {
	--$level;
      }
      elsif ($c eq ';' && $level == 0) {
	push @olines, $line;
	$last_index = $i;
	last LINE;
      }
    }
    push @olines, $line;
  }

  # print good lines to output file
  printf $fp "=== starting extracted code at input line %d:\n", $first_index + 1;
  print  $fp "$_" for @olines;
  printf $fp "=== ending extracted code at input line %d:\n", $last_index + 1;

  return $last_index;

} # extract_enum

sub extract_struct {
  # example:
  #   typedef struct (OR struct)
  #     {
  #       unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  #     } __sigset_t;

  my $lines_aref = shift @_; # \@lines
  my $i          = shift @_; # $i - current @lines index
  my $fp         = shift @_; # ouput file pointer

  my $nl = scalar @{$lines_aref};

  # we're at the first line of the struct
  # get all lines til the last
  my @olines = ();
  # track '{}' levels
  my $level = 0;
  my $first_index = $i;
  my $last_index;

 LINE:
  for (; $i < $nl; ++$i) {
    my $lnum = $i + 1;
    my $line = $lines_aref->[$i];
    my $len = length $line;
    for (my $j = 0; $j < $len; ++$j) {
      my $c = substr $line, $j, 1;
      if ($c eq '{') {
	++$level;
      }
      elsif ($c eq '}') {
	--$level;
      }
      elsif ($c eq ';' && $level == 0) {
	push @olines, $line;
	$last_index = $i;
	last LINE;
      }
    }
    push @olines, $line;
  }

  # print good lines to output file
  printf $fp "=== starting extracted code at input line %d:\n", $first_index + 1;
  print  $fp "$_" for @olines;
  printf $fp "=== ending extracted code at input line %d:\n", $last_index + 1;

  return $last_index;

} # extract_struct

=cut

# mandatory true return for a Perl module
1;
