import tkinter as tk
from tkinter import scrolledtext, filedialog, messagebox
import subprocess

def display_message():
    machine_code_display.insert(tk.END, "Machine code will be shown here...\n")
    errors_display.insert(tk.END, "Errors will be shown here...\n")
    memory_display.insert(tk.END, "Memory modified will be shown here...\n")
    register_display.insert(tk.END, "Register status will be shown here...\n")

def run_assembler():
    # Save code to a temporary file
    code = code_editor.get("1.0", tk.END)
    file_path = "Program.txt"
    with open(file_path, "w") as f:
        f.write(code)

    # Run the C++ backend (Windows executable)
    try:
        result = subprocess.run(["backend.exe", file_path], capture_output=True, text=True, shell=True)
        errors_display.delete("1.0", tk.END)
        errors_display.insert(tk.END, result.stderr)

        # Assume output is in format: "MEMORY: ...\nREGISTERS: ..."
        output_lines = result.stdout.split("\n")
        memory_display.delete("1.0", tk.END)
        machine_code_display.delete("1.0", tk.END)
        register_display.delete("1.0", tk.END)

        display_message()

        menu = 0
        r = 1
        for line in output_lines:
            if line.startswith("Machine Code:"):
                menu = 0
                continue
            elif line.startswith("Errors:"):
                menu = 1
                continue
            elif line.startswith("Registers:"):
                menu = 2
                continue
            elif line.startswith("Emulator Errors:"):
                menu = 3
                continue
            elif line.startswith("Memory:"):
                menu = 4
                continue

            if menu == 0:
                machine_code_display.insert(tk.END, line + "\n")
            elif menu == 1:
                errors_display.insert(tk.END, line + "\n")
            elif menu == 2:
                register_display.insert(tk.END, "r" + str(r) + ": " + line + " ")
                if r%10 == 0:
                    register_display.insert(tk.END, "\n")
                r += 1
            elif menu == 3:
                errors_display.insert(tk.END, line + "\n")
            elif menu == 4:
                memory_display.insert(tk.END, line + "\n")

    except Exception as e:
        messagebox.showerror("Execution Error", str(e))


# Create main window
root = tk.Tk()
root.title("Assembler-Emulator GUI")
root.geometry("800x600")

# Create a frame for horizontal layout
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.BOTH, expand=True)

# Configure grid to have two equal columns
top_frame.columnconfigure(0, weight=1)
top_frame.columnconfigure(1, weight=1)

# Code editor (Left Side)
code_editor = scrolledtext.ScrolledText(top_frame, height=20)
code_editor.grid(row=0, column=0, sticky="nsew")
code_editor.insert(tk.END, "//Code Editor for SimpleRisc\n")
code_editor.insert(tk.END, "//By default all memory locations and registers are 0\n")
code_editor.insert(tk.END, "//Register status is in decimal format and memory is in binary format\n")

# Machine Code display (Right Side)
machine_code_display = scrolledtext.ScrolledText(top_frame, height=20)
machine_code_display.grid(row=0, column=1, sticky="nsew")
machine_code_display.insert(tk.END, "Machine code will be shown here...\n")

# Run button
run_button = tk.Button(root, text="Run", command=run_assembler)
run_button.pack()

# Errors display
errors_display = scrolledtext.ScrolledText(root, height=5, fg="red")
errors_display.pack(fill=tk.BOTH, expand=True)
errors_display.insert(tk.END, "Errors will be shown here...\n")

# Memory display
memory_display = scrolledtext.ScrolledText(root, height=5)
memory_display.pack(fill=tk.BOTH, expand=True)
memory_display.insert(tk.END, "Memory modified will be shown here...\n")

# Register display
register_display = scrolledtext.ScrolledText(root, height=5)
register_display.pack(fill=tk.BOTH, expand=True)
register_display.insert(tk.END, "Register status will be shown here...\n")

root.mainloop()

