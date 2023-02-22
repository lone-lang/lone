define plv
  print-lone-value $arg0
  printf "\n"
end

define print-lone-value
  if ! $arg0
    printf "NULL"
  else
    set var $type = $arg0->type
    if $type == LONE_LIST
      if ($arg0->list.first == 0) && ($arg0->list.rest == 0)
        printf "nil"
      else
        print-lone-list $arg0
      end
    else
      if $type == LONE_BYTES
        print-lone-bytes $arg0
      else
        if $type == LONE_TEXT
          print-lone-text $arg0
        else
          if $type == LONE_SYMBOL
            print-lone-symbol $arg0
          else
            if $type == LONE_INTEGER
              print-lone-integer $arg0
            else
              if $type == LONE_POINTER
                print-lone-pointer $arg0
              end
            end
          end
        end
      end
    end
  end
end

define print-lone-list
  printf "("
  print-lone-list-recursive $arg0
  printf ")"
end

define print-lone-list-recursive
  set var $list = $arg0
  set var $first = $list->list.first
  set var $rest = $list->list.rest

  print-lone-value $first

  if $rest.type == LONE_LIST
    if !($rest->list.first == 0) && !($rest->list.rest == 0)
      printf " "
      print-lone-list-recursive $rest
    end
  else
    printf ". "
    print-lone-value $rest
  end
end

define print-lone-bytes
  printf "b["
  print-lone-memory $arg0
  printf "]"
end

define print-lone-text
  printf "t["
  print-lone-memory $arg0
  printf "]"
end

define print-lone-symbol
  printf "s["
  print-lone-memory $arg0
  printf "]"
end

define print-lone-memory
  set var $pointer = $arg0->bytes.pointer
  set var $count = $arg0->bytes.count
  output *$pointer@$count
end

define print-lone-integer
  output $arg0->integer
end

define print-lone-pointer
  output $arg0->pointer
end
