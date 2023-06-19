import subprocess

pages = '100'
arg_list = []
algo = "custom" 
program = "alpha"
page_faults = []
reads = []
writes = []

# setup input arguments 
for i in range(3, 100, 3):
    arg_list.append([pages, str(i), algo, program])

# run the program
for args in arg_list:
    print(f"Running ./virtmem {pages} {args[1]} {algo} {program}...")
    result = subprocess.run(['./virtmem'] + args, capture_output=True)
    output = result.stdout.decode()
            
    with open("output.txt", "w") as first:
        for line in output:
            first.write(line)

    counter = 0
    with open("output.txt", "r") as second:
        for line in second:
            line = line.rstrip()
            if line.startswith("alpha"):
                continue
            if line.startswith("Running"):
                continue
            elif counter == 0:
                page_faults.append(line)
                counter = 1
            elif counter == 1:
                reads.append(line)
                counter = 2
            elif counter == 2:
                writes.append(line)
                counter = 0

with open("custom_alpha_page_faults.txt", "w") as faults:
    for num in page_faults:
        faults.write(num)
        faults.write("\n")

with open("custom_alpha_reads.txt", "w") as reading:
    for num in reads:
        reading.write(num)
        reading.write("\n")

with open("custom_alpha_writes.txt", "w") as writing:
    for num in writes:
        writing.write(num)
        writing.write("\n")