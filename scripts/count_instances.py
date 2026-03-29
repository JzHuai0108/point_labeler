#!/usr/bin/env python3
"""Count number of instances per class in a .label file.

Label format: uint32 where lower 16 bits = class ID, upper 16 bits = instance ID.
Instance ID 0 means the point is not assigned to any instance.
"""

import sys
import numpy as np


def count_instances(label_file):
    data = np.fromfile(label_file, dtype=np.uint32)
    classes = data & 0xFFFF
    instances = (data >> 16) & 0xFFFF

    print(f"File: {label_file}")
    print(f"Total points: {len(data)}")
    print()

    unique_classes = np.unique(classes)
    for cls in unique_classes:
        mask = classes == cls
        cls_instances = instances[mask]
        # instance ID 0 means unlabeled/no instance
        unique_inst = np.unique(cls_instances[cls_instances > 0])
        n_points = np.sum(mask)
        n_instances = len(unique_inst)
        print(f"  Class {cls:5d}: {n_points:8d} points, {n_instances} instance(s)", end="")
        if n_instances > 0:
            counts = [int(np.sum(cls_instances == i)) for i in unique_inst]
            print(f"  {list(zip(unique_inst.tolist(), counts))}", end="")
        print()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path/to/file.label>")
        sys.exit(1)
    count_instances(sys.argv[1])
