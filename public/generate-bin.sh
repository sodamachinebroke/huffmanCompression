#!/bin/bash

# Array of sizes (in bytes) - Adjust as needed
sizes=(12 256 1024 8192 65536 524288 1048576 1073741824)

# Loop through the sizes, using a counter for filenames
counter=1
for size in "${sizes[@]}"; do
  # Generate filename
  filename="input${counter}.bin"

  # Generate random data and truncate
  head -c "$size" /dev/urandom > "$filename"

  # Verify size (optional)
  file_size=$(stat -c "%s" "$filename")
  echo "Generated $filename: $file_size bytes"

  # Increment counter
  counter=$((counter + 1))
done

echo "Finished generating random binary files."