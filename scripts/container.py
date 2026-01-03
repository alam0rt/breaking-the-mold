#!/usr/bin/env python3
"""
container.py - Common container structure for BLB asset parsing

This module provides a unified Container class for parsing and validating
TOC-based container structures found in Skullmonkeys BLB archives.

Container Format:
=================
    Offset  Size   Description
    ------  ----   -----------
    0x00    u32    Entry count
    0x04+   12×N   TOC entries (N = count)

    Each Entry (12 bytes):
      0x00   u32    Field A (type for segment TOC, flags for asset sub-TOC)
      0x04   u32    Size in bytes
      0x08   u32    Offset from container start

Validation Rules:
=================
    1. Entry count must be reasonable (< 1000)
    2. All offsets must be within container bounds
    3. All offset+size must not exceed container bounds
    4. Entries must not overlap with each other
    5. Entries must not overlap with the TOC header itself
    6. First entry offset should equal header size (4 + count * 12)
"""

import struct
from dataclasses import dataclass, field
from typing import List, Optional, Tuple


@dataclass
class ContainerEntry:
    """A single entry in a container's TOC."""
    index: int
    field_a: int      # Type (segment TOC) or Flags (asset sub-TOC)
    size: int         # Size in bytes
    offset: int       # Offset from container start
    
    @property
    def end_offset(self) -> int:
        """Calculate the end offset (offset + size)."""
        return self.offset + self.size
    
    def overlaps(self, other: "ContainerEntry") -> bool:
        """Check if this entry overlaps with another entry."""
        if self.index == other.index:
            return False  # Don't compare with self
        # Check if ranges overlap: [a.offset, a.end) vs [b.offset, b.end)
        return self.offset < other.end_offset and other.offset < self.end_offset
    
    def __repr__(self) -> str:
        return (f"ContainerEntry(idx={self.index}, field_a=0x{self.field_a:08X}, "
                f"size={self.size}, offset=0x{self.offset:X})")


@dataclass
class ValidationError:
    """Represents a validation error in a container."""
    error_type: str
    message: str
    entry_index: Optional[int] = None
    other_index: Optional[int] = None
    
    def __str__(self) -> str:
        if self.entry_index is not None:
            if self.other_index is not None:
                return f"[{self.error_type}] Entry {self.entry_index} vs {self.other_index}: {self.message}"
            return f"[{self.error_type}] Entry {self.entry_index}: {self.message}"
        return f"[{self.error_type}] {self.message}"


@dataclass
class Container:
    """
    A generic container structure with TOC-based entries.
    
    This class can represent:
    - Segment TOCs (Primary, Secondary, Tertiary)
    - Asset sub-TOCs (within 0x258, 0x259, 0x190 assets)
    
    Both use the same 12-byte entry format but interpret field_a differently.
    """
    
    data: bytes                     # Raw container data
    entries: List[ContainerEntry]   # Parsed TOC entries
    container_size: int             # Total container size (for validation)
    name: str = "Container"         # Name for error messages
    
    @classmethod
    def from_bytes(cls, data: bytes, name: str = "Container", 
                   expected_size: Optional[int] = None) -> "Container":
        """
        Parse a container from raw bytes.
        
        Args:
            data: Raw container data (must include full container)
            name: Name for error messages
            expected_size: Expected total size (for validation)
            
        Returns:
            Parsed Container instance
        """
        if len(data) < 4:
            raise ValueError(f"{name}: Data too short for header ({len(data)} bytes)")
        
        count = struct.unpack_from('<I', data, 0)[0]
        
        # Sanity check on count
        if count > 10000:
            raise ValueError(f"{name}: Entry count too large ({count})")
        
        header_size = 4 + count * 12
        
        if len(data) < header_size:
            raise ValueError(f"{name}: Data too short for TOC "
                           f"({len(data)} < {header_size} bytes)")
        
        entries = []
        for i in range(count):
            offset = 4 + i * 12
            field_a, size, entry_offset = struct.unpack_from('<III', data, offset)
            entries.append(ContainerEntry(
                index=i,
                field_a=field_a,
                size=size,
                offset=entry_offset,
            ))
        
        container_size = expected_size if expected_size else len(data)
        
        return cls(
            data=data,
            entries=entries,
            container_size=container_size,
            name=name,
        )
    
    @property
    def count(self) -> int:
        """Number of entries in the container."""
        return len(self.entries)
    
    @property
    def header_size(self) -> int:
        """Size of the TOC header (4 + count * 12)."""
        return 4 + self.count * 12
    
    def get_entry_data(self, entry: ContainerEntry) -> bytes:
        """Get the raw data for a specific entry."""
        return self.data[entry.offset:entry.end_offset]
    
    def validate(self) -> List[ValidationError]:
        """
        Validate the container structure.
        
        Returns:
            List of validation errors (empty if valid)
        """
        errors = []
        
        # Rule 1: Entry count sanity (already checked in from_bytes)
        
        # Rule 2 & 3: Bounds checking
        for entry in self.entries:
            if entry.offset < 0:
                errors.append(ValidationError(
                    "NEGATIVE_OFFSET",
                    f"Offset is negative: {entry.offset}",
                    entry_index=entry.index
                ))
            
            if entry.offset > self.container_size:
                errors.append(ValidationError(
                    "OFFSET_OUT_OF_BOUNDS",
                    f"Offset 0x{entry.offset:X} exceeds container size 0x{self.container_size:X}",
                    entry_index=entry.index
                ))
            
            if entry.end_offset > self.container_size:
                errors.append(ValidationError(
                    "END_OUT_OF_BOUNDS",
                    f"End offset 0x{entry.end_offset:X} exceeds container size 0x{self.container_size:X}",
                    entry_index=entry.index
                ))
        
        # Rule 4: No overlapping entries
        # Note: We distinguish between:
        #   - DUPLICATE: Entries with identical ranges (intentional asset sharing)
        #   - OVERLAP: Entries with partial overlap (likely an error)
        for i, entry_a in enumerate(self.entries):
            for entry_b in self.entries[i+1:]:
                # Check for exact duplicate (same offset AND same size)
                if entry_a.offset == entry_b.offset and entry_a.size == entry_b.size:
                    # This is intentional asset sharing, not an error
                    # Just note it as INFO, not an error
                    errors.append(ValidationError(
                        "DUPLICATE",
                        f"Duplicate reference to same data: [{entry_a.offset:X}-{entry_a.end_offset:X}]",
                        entry_index=entry_a.index,
                        other_index=entry_b.index
                    ))
                elif entry_a.overlaps(entry_b):
                    # True partial overlap - this is a real error
                    errors.append(ValidationError(
                        "OVERLAP",
                        f"Entries overlap: [{entry_a.offset:X}-{entry_a.end_offset:X}] "
                        f"vs [{entry_b.offset:X}-{entry_b.end_offset:X}]",
                        entry_index=entry_a.index,
                        other_index=entry_b.index
                    ))
        
        # Rule 5: Entries must not overlap with TOC header
        for entry in self.entries:
            if entry.offset < self.header_size and entry.size > 0:
                errors.append(ValidationError(
                    "HEADER_OVERLAP",
                    f"Entry offset 0x{entry.offset:X} overlaps with TOC header "
                    f"(ends at 0x{self.header_size:X})",
                    entry_index=entry.index
                ))
        
        # Rule 6: First entry offset should equal header size (warning, not error)
        if self.entries:
            first_offset = self.entries[0].offset
            if first_offset != self.header_size:
                # This might be valid if there's padding, so just note it
                errors.append(ValidationError(
                    "FIRST_OFFSET_MISMATCH",
                    f"First entry offset 0x{first_offset:X} != header size 0x{self.header_size:X} "
                    f"(gap of {first_offset - self.header_size} bytes)",
                    entry_index=0
                ))
        
        return errors
    
    def is_valid(self) -> bool:
        """Check if the container is valid (no critical errors)."""
        errors = self.validate()
        # FIRST_OFFSET_MISMATCH and DUPLICATE are warnings, not critical errors
        critical_errors = [e for e in errors 
                          if e.error_type not in ("FIRST_OFFSET_MISMATCH", "DUPLICATE")]
        return len(critical_errors) == 0
    
    def get_coverage(self) -> Tuple[int, int, float]:
        """
        Calculate how much of the container is covered by entries.
        
        Returns:
            (covered_bytes, total_bytes, percentage)
        """
        covered = sum(e.size for e in self.entries)
        return covered, self.container_size, (covered / self.container_size * 100) if self.container_size > 0 else 0
    
    def print_summary(self) -> None:
        """Print a summary of the container structure."""
        print(f"{self.name}")
        print("=" * 60)
        print(f"Entry count: {self.count}")
        print(f"Header size: {self.header_size} bytes")
        print(f"Total size:  {self.container_size} bytes")
        print()
        
        print(f"{'#':>3} {'FieldA':>10} {'Size':>10} {'Offset':>10} {'End':>10}")
        print("-" * 60)
        
        for entry in self.entries:
            print(f"{entry.index:3} 0x{entry.field_a:08X} {entry.size:10,} "
                  f"0x{entry.offset:08X} 0x{entry.end_offset:08X}")
        
        print()
        
        # Validate and show errors
        errors = self.validate()
        if errors:
            print(f"Validation: {len(errors)} issue(s)")
            for error in errors:
                print(f"  {error}")
        else:
            print("Validation: OK")
        
        # Coverage
        covered, total, pct = self.get_coverage()
        print(f"Coverage: {covered:,} / {total:,} bytes ({pct:.1f}%)")


# Asset type constants
CONTAINER_ASSET_TYPES = {0x258, 0x259, 0x190}  # Types that have sub-TOCs
RAW_ASSET_TYPES = {0x25A, 0x064, 0x065, 0x12C, 0x12D, 0x12E, 0x191}  # Raw data types


def is_container_type(asset_type: int) -> bool:
    """Check if an asset type is a container (has sub-TOC)."""
    return asset_type in CONTAINER_ASSET_TYPES


class SegmentTOC(Container):
    """
    Segment-level TOC (Primary, Secondary, Tertiary data).
    
    In segment TOCs, field_a is the asset type (0x258, 0x259, 0x25A, etc.)
    """
    
    @classmethod
    def from_bytes(cls, data: bytes, name: str = "SegmentTOC",
                   expected_size: Optional[int] = None) -> "SegmentTOC":
        """Parse a segment TOC from raw bytes."""
        container = super().from_bytes(data, name, expected_size)
        return cls(
            data=container.data,
            entries=container.entries,
            container_size=container.container_size,
            name=name,
        )
    
    def get_asset_types(self) -> List[int]:
        """Get list of asset types in this segment."""
        return [e.field_a for e in self.entries]
    
    def get_entry_by_type(self, asset_type: int) -> Optional[ContainerEntry]:
        """Get entry by asset type."""
        for entry in self.entries:
            if entry.field_a == asset_type:
                return entry
        return None


class AssetSubTOC(Container):
    """
    Asset-level sub-TOC (within container assets like 0x258, 0x259, 0x190).
    
    In asset sub-TOCs, field_a is flags/metadata (exact meaning varies by asset type).
    """
    
    asset_type: int = 0  # The parent asset type (0x258, 0x259, etc.)
    
    @classmethod
    def from_bytes(cls, data: bytes, asset_type: int = 0, name: str = "AssetSubTOC",
                   expected_size: Optional[int] = None) -> "AssetSubTOC":
        """Parse an asset sub-TOC from raw bytes."""
        container = super().from_bytes(data, name, expected_size)
        instance = cls(
            data=container.data,
            entries=container.entries,
            container_size=container.container_size,
            name=name,
        )
        instance.asset_type = asset_type
        return instance


def validate_segment_with_assets(segment_data: bytes, segment_name: str = "Segment") -> List[ValidationError]:
    """
    Validate a complete segment including its container assets.
    
    This parses the segment TOC, then for each container-type asset,
    parses and validates its sub-TOC.
    
    Args:
        segment_data: Raw segment data
        segment_name: Name for error messages
        
    Returns:
        List of all validation errors
    """
    all_errors = []
    
    # Parse and validate segment TOC
    try:
        segment = SegmentTOC.from_bytes(segment_data, name=segment_name)
    except ValueError as e:
        return [ValidationError("PARSE_ERROR", str(e))]
    
    segment_errors = segment.validate()
    for error in segment_errors:
        error.message = f"Segment: {error.message}"
    all_errors.extend(segment_errors)
    
    # For each container asset, validate its sub-TOC
    for entry in segment.entries:
        if is_container_type(entry.field_a):
            asset_data = segment.get_entry_data(entry)
            asset_name = f"{segment_name}/Asset[{entry.index}](type=0x{entry.field_a:X})"
            
            try:
                asset = AssetSubTOC.from_bytes(
                    asset_data,
                    asset_type=entry.field_a,
                    name=asset_name,
                    expected_size=entry.size
                )
                asset_errors = asset.validate()
                all_errors.extend(asset_errors)
            except ValueError as e:
                all_errors.append(ValidationError(
                    "ASSET_PARSE_ERROR",
                    f"Failed to parse {asset_name}: {e}",
                    entry_index=entry.index
                ))
    
    return all_errors


# For testing
if __name__ == "__main__":
    print("Container module loaded. Run test_containers.py for validation tests.")
