# all credits go to ChatGPT for vibe coding this for me

import os
import re

# List of directories to search
directories = [
    "./engine",
    "./game",
    "./badtl",
    # Add more directories here
]

output_file = "todos.md"

# Pattern to detect TODO comments and capture the text
todo_pattern = re.compile(r"//\s*TODO\(.*?\):\s*(.*)")

# Pattern to detect struct definitions
struct_pattern = re.compile(r"^\s*struct\s+\w+")

context_lines_count = 3  # maximum number of lines to include after TODO

with open(output_file, "w", encoding="utf-8") as md_file:
    md_file.write("# TODO Report\n\n")

    for directory in directories:
        md_file.write(f"## Directory: {directory}\n\n")
        for root, dirs, files in os.walk(directory):
            # Sort files for consistent order
            files.sort()
            for file in files:
                file_path = os.path.join(root, file)
                relative_path = os.path.relpath(file_path, directory)

                try:
                    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                        lines = f.readlines()
                        i = 0
                        while i < len(lines):
                            line = lines[i].strip()
                            match = todo_pattern.match(line)
                            if match:
                                start_line_number = i + 1
                                current_text_lines = [match.group(1)]
                                i += 1

                                # Collect following // comment lines that belong to this TODO
                                while i < len(lines):
                                    stripped = lines[i].strip()
                                    if stripped.startswith(
                                        "//"
                                    ) and not todo_pattern.match(stripped):
                                        text = stripped[2:].strip()
                                        if text:
                                            current_text_lines.append(text)
                                        i += 1
                                    else:
                                        break

                                todo_text = " ".join(current_text_lines)

                                # Collect context lines with brace tracking
                                context_lines = []
                                context_lines_added = 0
                                brace_balance = 0
                                j = i
                                while (
                                    j < len(lines)
                                    and context_lines_added < context_lines_count
                                ):
                                    context_line = lines[j].rstrip()
                                    stripped_context = context_line.strip()

                                    if stripped_context == "":
                                        break
                                    if todo_pattern.match(stripped_context):
                                        break
                                    if struct_pattern.match(stripped_context):
                                        context_lines.append(context_line)
                                        break

                                    brace_balance += stripped_context.count("{")
                                    closing_braces = stripped_context.count("}")
                                    if closing_braces > 0:
                                        if brace_balance > 0:
                                            context_lines.append(context_line)
                                            brace_balance -= closing_braces
                                            break
                                        else:
                                            break

                                    context_lines.append(context_line)
                                    context_lines_added += 1
                                    j += 1

                                # Write TODO to Markdown
                                md_file.write(f"### **TODO:** {todo_text}\n\n")
                                md_file.write(
                                    f"{relative_path} (line {start_line_number})\n\n"
                                )
                                if context_lines:
                                    md_file.write("**Context:**\n\n")
                                    md_file.write("```cpp\n")
                                    md_file.write("\n".join(context_lines) + "\n")
                                    md_file.write("```\n\n")
                                md_file.write("---\n\n")
                            else:
                                i += 1

                except Exception as e:
                    print(f"Error reading {file_path}: {e}")

print(f"Markdown file '{output_file}' created.")
