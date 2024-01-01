import os
import argparse
import re
import matplotlib.pyplot as plt


if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Process some parametars.')
  parser.add_argument("--log_file", help="Path to the log file")
  args = parser.parse_args()


  decode_time = []
  
  text = "frames is 6.30248 ms"
  with open(args.log_file, 'r') as file:
    for line in file:
        match = re.search(r'frames is (\d+\.\d+) ms', line)
        if match:
            extracted_number = float(match.group(1))
            decode_time.append(extracted_number)
 

  plt.plot(decode_time[20:])
  plt.xlabel("Frame/per 100 frames")
  plt.ylabel("Decode Time (ms)")
  plt.title("Decode Time per 100 Frames")
  plt.grid(True)
  plt.savefig("result.jpeg")