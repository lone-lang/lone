define print-lone-memory
  set var $memory = $arg0
  printf "[ "
  if $memory->free
    printf "."
  else
    printf "!"
  end
  printf " "
  output $memory->size
  printf " ]"
end

define lone-memory-walk
  set var $memory = $arg0->memory
  set var $total = 0
  set var $free = 0
  while $memory
    set var $total += sizeof(struct lone_memory) + $memory->size
    if $memory->free
      set var $free += $memory->size
    end
    print-lone-memory $memory
    printf " "
    set var $memory = $memory->next
  end
  set var $used = $total - $free
  output $free
  printf " free, "
  output $used
  printf " used, "
  output $total
  printf " total\n"
end

define plv
  print-lone-value $arg0
  printf "\n"
end

define print-lone-value
  if $arg0
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
              else
                if $type == LONE_TABLE
                  print-lone-table $arg0
                else
                  if $type == LONE_FUNCTION
                    print-lone-function $arg0
                  else
                    if $type == LONE_PRIMITIVE
                      print-lone-primitive $arg0
                    end
                  end
                end
              end
            end
          end
        end
      end
    end
  else
    printf "NULL"
  end
end

define print-lone-function
  set var $f = $arg0->function
  printf "f["
  print-lone-value $f.arguments
  printf " = "
  print-lone-value $f.code
  printf "]"
end

define print-lone-primitive
  printf "p["
  p $arg0->primitive
  printf "]"
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

define print-lone-table
  set var $table = $arg0->table
  printf "table["
  output $table.count
  printf ", "
  output $table.capacity
  printf "]={\n"
  set var $i = 0
  while $i < $table.capacity
    set var $entry = &$table.entries[$i]
    if $entry->key
      printf "\t"
      output $i
      printf "\t=>\t"
      print-lone-table-entry $entry
      printf "\n"
    end
    set var $i = $i + 1
  end
  printf "}"
end

define print-lone-table-entry
  set var $key = $arg0->key
  set var $value = $arg0->value
  print-lone-value $key
  printf " => "
  print-lone-value $value
end

define print-lone-bytes
  printf "b["
  print-lone-buffer $arg0
  printf "]"
end

define print-lone-text
  printf "t["
  print-lone-buffer $arg0
  printf "]"
end

define print-lone-symbol
  printf "s["
  print-lone-buffer $arg0
  printf "]"
end

define print-lone-buffer
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
