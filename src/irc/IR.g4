grammar IR;

module: decl+ EOF;
decl: function;

function: FUNCTION '(' ')' '{' '}';

FUNCTION: 'function';
STRUCT:   'struct';

