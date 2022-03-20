//
// Created by byjtew on 17/03/2022.
//

void seg() {
  char *str;
  str = "Text";
  // editing read only memory
  *(str + 1) = 'n';
}

int main() {
  seg();
  return 0;
}