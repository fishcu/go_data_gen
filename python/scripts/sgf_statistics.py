import os
import re
from collections import defaultdict
from typing import Dict, Set


def parse_rules_string(rules_str: str) -> Dict[str, str]:
    """Parse a rules string into a dictionary of rule components."""
    # Remove RU[] wrapper if present
    rules_str = rules_str.strip('RU[]')

    # Initialize result dictionary
    rules = {}

    # Use regex to find patterns of lowercase+UPPERCASE or lowercase+number
    pattern = r'([a-z]+)([A-Z]+|\d+)'
    matches = re.finditer(pattern, rules_str)

    for match in matches:
        key, value = match.groups()
        rules[key] = value

    return rules


def analyze_sgf_rules(file_path: str) -> Dict[str, str]:
    """Extract rules from an SGF file."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        rules = {}

        # Find RU[] tag
        rules_match = re.search(r'RU\[[^\]]*\]', content)
        if rules_match:
            rules.update(parse_rules_string(rules_match.group()))

        # Find SZ[] tag
        size_match = re.search(r'SZ\[([^\]]*)\]', content)
        if size_match:
            size = size_match.group(1)
            if ':' in size:  # rectangular board
                width, height = map(int, size.split(':'))
                # Ensure larger dimension comes first
                width, height = max(width, height), min(width, height)
                rules['sz'] = f"{width}x{height}"
            else:  # square board
                rules['sz'] = f"{size}x{size}"  # Convert "19" to "19x19"

        return rules
    except Exception as e:
        print(f"Error processing {file_path}: {str(e)}")

    return {}


def collect_sgf_rules_statistics(directory: str) -> Dict[str, Dict[str, int]]:
    """Collect statistics about rules usage across all SGF files in a directory."""
    # Dictionary to store statistics for each rule type
    stats = defaultdict(lambda: defaultdict(int))
    total_files = 0
    files_with_rules = 0

    # Walk through directory
    for root, _, files in os.walk(directory):
        for file in files:
            if file.lower().endswith('.sgf'):
                total_files += 1
                file_path = os.path.join(root, file)

                rules = analyze_sgf_rules(file_path)
                if rules:
                    files_with_rules += 1
                    for key, value in rules.items():
                        stats[key][value] += 1

    return stats, total_files, files_with_rules


def print_statistics(stats: Dict[str, Dict[str, int]], total_files: int, files_with_rules: int):
    """Print formatted statistics."""
    print("\nSGF Rules Statistics")
    print("===================")
    print(f"Total SGF files analyzed: {total_files}")
    print(f"Files with rules (RU[]) tag: {files_with_rules}")
    print(f"Files without rules: {total_files - files_with_rules}")
    print("\nRules Breakdown:")
    print("---------------")

    for rule_type, values in sorted(stats.items()):
        print(f"\n{rule_type}:")
        total = sum(values.values())

        # Special sorting for 'sz' field using natural sort
        if rule_type == 'sz':
            # Convert "WxH" to tuple of integers for sorting
            sorted_items = sorted(
                values.items(),
                key=lambda x: tuple(map(int, x[0].split('x'))),
                reverse=True
            )
        else:
            sorted_items = sorted(values.items())

        for value, count in sorted_items:
            percentage = (count / files_with_rules) * \
                100 if files_with_rules > 0 else 0
            print(f"  {value}: {count} ({percentage:.1f}%)")


def main():
    import sys

    if len(sys.argv) != 2:
        print("Usage: python sgf_rules_statistics.py <directory>")
        sys.exit(1)

    directory = sys.argv[1]
    if not os.path.isdir(directory):
        print(f"Error: {directory} is not a valid directory")
        sys.exit(1)

    stats, total_files, files_with_rules = collect_sgf_rules_statistics(
        directory)
    print_statistics(stats, total_files, files_with_rules)


if __name__ == "__main__":
    main()
