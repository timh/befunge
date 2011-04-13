25*    v                "push 10 to the stack"
       >:.         v    "loop start: clone top item/curnum, then output it"
           v       <
           > 1-v        "push 1, swap (with current number), then subtract"
   v           <
   >  :|                "clone for compare, then if non-zero, loop, else end.."
       @
