#!/usr/bin/env python3
"""Convert pkDex XML data files to JSON format.

Usage:
    python3 tools/convert_xml_to_json.py

Reads all XML files in resources/data/ and writes corresponding JSON files.
"""

import json
import os
import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(SCRIPT_DIR, "..", "resources", "data")


def convert_regions(xml_path, json_path):
    """Convert regions.xml to regions.json (a simple JSON array)."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    regions = [elem.text.strip() for elem in root.findall("region") if elem.text]
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(regions, f, indent=2, ensure_ascii=False)
    print(f"  {os.path.basename(xml_path)} -> {os.path.basename(json_path)} ({len(regions)} regions)")


def convert_region(xml_path, json_path):
    """Convert a region XML file to JSON (array-of-objects format)."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    region = root.get("region", "unknown")

    pokemon_list = []
    for elem in root.findall("pokemon"):
        entry = {
            "id": elem.get("id", ""),
            "name": elem.get("name", ""),
            "regionalDexNumber": elem.get("regionalDexNumber", ""),
            "type": elem.get("type", ""),
        }

        shiny_locked = elem.get("shinyLocked", "false")
        if shiny_locked == "true":
            entry["shinyLocked"] = True

        evolution = elem.find("evolution")
        entry["evolution"] = evolution.text.strip() if evolution is not None and evolution.text else ""

        locations = elem.find("locations")
        entry["locations"] = locations.text.strip() if locations is not None and locations.text else ""

        exclusive_version = elem.find("exclusiveVersion")
        entry["exclusiveVersion"] = exclusive_version.text.strip() if exclusive_version is not None and exclusive_version.text else "-"

        pokemon_list.append(entry)

    data = {"region": region, "pokemon": pokemon_list}
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"  {os.path.basename(xml_path)} -> {os.path.basename(json_path)} ({len(pokemon_list)} pokemon)")


def main():
    print(f"Converting XML data files in: {DATA_DIR}\n")

    regions_xml = os.path.join(DATA_DIR, "regions.xml")
    if os.path.exists(regions_xml):
        convert_regions(regions_xml, os.path.join(DATA_DIR, "regions.json"))
    else:
        print("  WARNING: regions.xml not found")

    for filename in sorted(os.listdir(DATA_DIR)):
        if filename.endswith(".xml") and filename != "regions.xml":
            xml_path = os.path.join(DATA_DIR, filename)
            json_path = os.path.join(DATA_DIR, filename.replace(".xml", ".json"))
            convert_region(xml_path, json_path)

    print("\nDone.")


if __name__ == "__main__":
    main()
