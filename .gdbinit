define print-lone-bytes
  set var $pointer = $arg0->pointer
  set var $count = $arg0->count
  output *$pointer@$count
end

define print-lone-list
  set var $list = $arg0
  printf "("
  while $list
    set var $first = $list->list.first
    if ! $first
      printf " nil "
    end
    if $first && $first.type == LONE_LIST
      printf " "
      print-lone-list $first.list
      printf " "
    end
    if $first && $first.type == LONE_BYTES
      printf " "
      print-lone-bytes $first.bytes
      printf " "
    end
    set var $list = $list->list.rest
  end
  printf ")"
end

define pll
  print-lone-list $arg0
  printf "\n"
end
