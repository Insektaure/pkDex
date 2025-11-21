#!/bin/bash

LANG="jp"

JSON_FILE="data.json"
JSON_FILE_TRANSLATED="data_translated.json"
CSV_FILE="pokemon_names_en_${LANG}.csv"
CSV_FILE_TYPE="type_${LANG}.csv"
TMP_FILE="tmp.json"
TMP_FILE_TYPE="tmp_type.json"

cp "$JSON_FILE" "$TMP_FILE"

#while IFS=: read -r EN FR; do
#    if grep -q "\"$EN\"" "$TMP_FILE"; then
#        echo "Replacing '$EN' with '$FR'"
#        sed -i "s/\"$EN\"/\"$FR\"/g" "$TMP_FILE"
#    fi
#done < "$CSV_FILE"

while IFS=: read -r EN FR; do
    if grep -q "$EN" "$TMP_FILE"; then
        echo "Replacing '$EN' with '$FR'"
        # Escape special characters in EN and FR for sed
        EN_ESC=$(printf '%s\n' "$EN" | sed 's/[]\/$*.^[]/\\&/g')
        FR_ESC=$(printf '%s\n' "$FR" | sed 's/[&/\]/\\&/g')
        sed -i "s/$EN_ESC/$FR_ESC/g" "$TMP_FILE"
    fi
done < "$CSV_FILE"

cp "$TMP_FILE" "$TMP_FILE_TYPE"

while IFS=: read -r EN FR; do
    if grep -q "$EN" "$TMP_FILE_TYPE"; then
        echo "Replacing '$EN' with '$FR'"
        # Escape special characters in EN and FR for sed
        EN_ESC=$(printf '%s\n' "$EN" | sed 's/[]\/$*.^[]/\\&/g')
        FR_ESC=$(printf '%s\n' "$FR" | sed 's/[&/\]/\\&/g')
        sed -i "s/$EN_ESC/$FR_ESC/g" "$TMP_FILE_TYPE"
    fi
done < "$CSV_FILE_TYPE"

mv "$TMP_FILE_TYPE" "$JSON_FILE_TRANSLATED"
rm "$TMP_FILE"
echo "Replacement complete."
