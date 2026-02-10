# all credits go to ChatGPT for vibe coding this for me

import os
import re

# List of directories to search
directories = [
    "./src",
    "./shaders",
    "./data",
]

output_file = "source_todos.md"
context_lines_count = 3

# =========================
# Helpers
# =========================


def is_cpp_file(path: str) -> bool:
    return path.endswith((".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"))


# struct Name
struct_pattern = re.compile(r"^\s*struct\s+\w+")
note_pattern = re.compile(r"NOTE\s*:")

# =========================
# Main Logic
# =========================

with open(output_file, "w", encoding="utf-8") as md_file:
    md_file.write("# TODO Report\n\n")

    for directory in directories:
        md_file.write(f"## Directory: {directory}\n\n")

        for root, dirs, files in os.walk(directory):
            files.sort()

            for file in files:
                file_path = os.path.join(root, file)
                relative_path = os.path.relpath(file_path, directory)

                try:
                    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                        lines = f.readlines()

                    # -------------------------
                    # Comment style per file
                    # -------------------------
                    if is_cpp_file(file_path):
                        todo_pattern = re.compile(r"//\s*TODO(?:\(.*?\))?:\s*(.*)")
                        comment_line_pattern = re.compile(r"^\s*//\s*(.*)")
                    else:
                        todo_pattern = re.compile(r"#\s*TODO(?:\(.*?\))?:\s*(.*)")
                        comment_line_pattern = re.compile(r"^\s*#\s*(.*)")

                    i = 0
                    while i < len(lines):
                        stripped = lines[i].strip()
                        match = todo_pattern.match(stripped)

                        if match:
                            # -------------------------
                            # Collect TODO text
                            # -------------------------
                            start_line_number = i + 1
                            todo_text_lines = [match.group(1)]
                            i += 1

                            # Continuation comment lines
                            while i < len(lines):
                                cont = comment_line_pattern.match(lines[i].strip())
                                if not cont:
                                    break

                                text = cont.group(1).strip()

                                # NOTE terminates TODO text collection
                                if note_pattern.match(text):
                                    i += 1
                                    break

                                # Do not start a new TODO
                                if todo_pattern.match(lines[i].strip()):
                                    break

                                if text:
                                    todo_text_lines.append(text)

                                i += 1

                            todo_text = " ".join(todo_text_lines)

                            # -------------------------
                            # Collect smart context
                            # -------------------------
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

                                # Stop conditions
                                if stripped_context == "":
                                    break
                                if todo_pattern.match(stripped_context):
                                    break

                                # struct definition: include & stop
                                if struct_pattern.match(stripped_context):
                                    context_lines.append(context_line)
                                    break

                                # Brace tracking
                                opening = stripped_context.count("{")
                                closing = stripped_context.count("}")
                                brace_balance += opening

                                if closing > 0:
                                    if brace_balance > 0:
                                        context_lines.append(context_line)
                                        brace_balance -= closing
                                        break
                                    else:
                                        break

                                context_lines.append(context_line)
                                context_lines_added += 1
                                j += 1

                            # -------------------------
                            # Write Markdown
                            # -------------------------
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
