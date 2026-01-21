"""Splat configuration file management."""

import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional, Any, List, Tuple

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq


# Constants for address conversion
RAM_BASE = 0x80010000
FILE_OFFSET = 0x800


def vram_to_rom(address: int) -> int:
    """Convert VRAM address to ROM file offset."""
    return (address - RAM_BASE) + FILE_OFFSET


def rom_to_vram(offset: int) -> int:
    """Convert ROM file offset to VRAM address."""
    return (offset - FILE_OFFSET) + RAM_BASE


@dataclass
class ValidationError:
    """A validation error or warning."""
    level: str  # 'error' or 'warning'
    message: str
    segment_index: Optional[int] = None
    segment_name: Optional[str] = None
    
    def __str__(self) -> str:
        loc = ""
        if self.segment_index is not None:
            loc = f"[segment {self.segment_index}]"
            if self.segment_name:
                loc = f"[segment {self.segment_index}: {self.segment_name}]"
        return f"{self.level.upper()}: {loc} {self.message}"


@dataclass
class ValidationResult:
    """Result of validating a splat config."""
    errors: List[ValidationError] = field(default_factory=list)
    warnings: List[ValidationError] = field(default_factory=list)
    
    @property
    def is_valid(self) -> bool:
        """Config is valid if there are no errors."""
        return len(self.errors) == 0
    
    def add_error(self, message: str, segment_index: Optional[int] = None, 
                  segment_name: Optional[str] = None) -> None:
        self.errors.append(ValidationError('error', message, segment_index, segment_name))
    
    def add_warning(self, message: str, segment_index: Optional[int] = None,
                    segment_name: Optional[str] = None) -> None:
        self.warnings.append(ValidationError('warning', message, segment_index, segment_name))
    
    def print_results(self) -> None:
        """Print all errors and warnings."""
        for err in self.errors:
            print(f"❌ {err}")
        for warn in self.warnings:
            print(f"⚠️  {warn}")


@dataclass
class SegmentInfo:
    """Information about an existing segment."""
    start: int  # ROM offset
    end: Optional[int]  # ROM offset (None if last segment)
    seg_type: str
    name: Optional[str]
    index: int  # Index in subsegments list
    
    @property
    def vram_start(self) -> int:
        return rom_to_vram(self.start)
    
    @property
    def vram_end(self) -> Optional[int]:
        if self.end is not None:
            return rom_to_vram(self.end)
        return None


class HexInt(int):
    """Integer that formats as hex in YAML."""
    pass


class SplatConfig:
    """Manager for splat YAML configuration files."""
    
    def __init__(self, config_path: Path):
        self.config_path = config_path
        self.yaml = YAML()
        self.yaml.preserve_quotes = True
        self.yaml.width = 120
        self._data: Optional[CommentedMap] = None
        
        # Register hex int representer
        def hex_representer(dumper, data):
            return dumper.represent_scalar('tag:yaml.org,2002:int', f'0x{data:X}')
        self.yaml.representer.add_representer(HexInt, hex_representer)
    
    def load(self) -> None:
        """Load the configuration file."""
        with open(self.config_path, 'r') as f:
            self._data = self.yaml.load(f)
    
    def save(self) -> None:
        """Save the configuration file."""
        if self._data is None:
            raise RuntimeError("No configuration loaded")
        with open(self.config_path, 'w') as f:
            self.yaml.dump(self._data, f)
    
    @property
    def data(self) -> CommentedMap:
        """Get the loaded configuration data."""
        if self._data is None:
            raise RuntimeError("No configuration loaded")
        return self._data
    
    def get_main_segment(self) -> Optional[CommentedMap]:
        """Find the main code segment."""
        for segment in self.data.get('segments', []):
            if isinstance(segment, dict) and segment.get('name') == 'main':
                return segment
        return None
    
    def get_subsegments(self) -> Optional[CommentedSeq]:
        """Get the subsegments list from the main segment."""
        main = self.get_main_segment()
        if main:
            return main.get('subsegments')
        return None
    
    def _get_segment_start(self, seg) -> int:
        """Extract start offset from a segment (list or dict format)."""
        if isinstance(seg, list):
            return seg[0]
        elif isinstance(seg, dict):
            return seg.get('start', 0)
        else:
            return seg
    
    def _get_segment_type(self, seg) -> str:
        """Extract type from a segment (list or dict format)."""
        if isinstance(seg, list):
            return seg[1] if len(seg) > 1 else 'asm'
        elif isinstance(seg, dict):
            return seg.get('type', 'asm')
        else:
            return 'end'
    
    def _get_segment_name(self, seg) -> Optional[str]:
        """Extract name from a segment (list or dict format)."""
        if isinstance(seg, list):
            return seg[2] if len(seg) > 2 else None
        elif isinstance(seg, dict):
            return seg.get('name')
        else:
            return None
    
    def parse_subsegments(self) -> List[SegmentInfo]:
        """Parse all subsegments into SegmentInfo objects."""
        subsegments = self.get_subsegments()
        if not subsegments:
            return []
        
        segments: List[SegmentInfo] = []
        
        for i, seg in enumerate(subsegments):
            segments.append(SegmentInfo(
                start=self._get_segment_start(seg),
                end=None,  # Will be filled in below
                seg_type=self._get_segment_type(seg),
                name=self._get_segment_name(seg),
                index=i
            ))
        
        # Fill in end offsets
        for i in range(len(segments) - 1):
            segments[i].end = segments[i + 1].start
        
        return segments
    
    def find_segment_at(self, rom_offset: int) -> Optional[SegmentInfo]:
        """Find the segment containing a ROM offset."""
        segments = self.parse_subsegments()
        for seg in segments:
            if seg.start <= rom_offset:
                if seg.end is None or rom_offset < seg.end:
                    return seg
        return None
    
    def find_insertion_index(self, rom_offset: int) -> int:
        """Find the index where a new segment should be inserted."""
        subsegments = self.get_subsegments()
        if not subsegments:
            return 0
        
        for i, seg in enumerate(subsegments):
            seg_start = self._get_segment_start(seg)
            if seg_start > rom_offset:
                return i
        
        return len(subsegments)
    
    def check_overlap_with_c(self, start: int, end: int) -> Optional[SegmentInfo]:
        """
        Check if a range overlaps with existing C segments.
        Returns the overlapping segment if found.
        """
        segments = self.parse_subsegments()
        for seg in segments:
            if seg.seg_type == 'c':
                seg_end = seg.end if seg.end else float('inf')
                # Check for overlap
                if start < seg_end and end > seg.start:
                    return seg
        return None
    
    def segment_exists_at(self, rom_offset: int) -> bool:
        """Check if a segment already starts at this exact offset."""
        subsegments = self.get_subsegments()
        if not subsegments:
            return False
        
        for seg in subsegments:
            if self._get_segment_start(seg) == rom_offset:
                return True
        return False
    
    def create_dict_segment(self, start: int, seg_type: str, name: Optional[str] = None) -> CommentedMap:
        """Create a segment in dictionary format."""
        seg = CommentedMap()
        
        if name:
            seg['name'] = name
        seg['type'] = seg_type
        seg['start'] = HexInt(start)
        
        return seg
    
    def create_list_segment(self, start: int, seg_type: str, name: Optional[str] = None) -> CommentedSeq:
        """Create a segment in list format (for compatibility)."""
        seg = CommentedSeq()
        seg.fa.set_flow_style()
        
        seg.append(HexInt(start))
        seg.append(seg_type)
        if name:
            seg.append(name)
        
        return seg
    
    def add_segment(
        self,
        start: int,
        seg_type: str,
        name: Optional[str] = None,
        end: Optional[int] = None,
        use_dict_format: bool = True
    ) -> bool:
        """
        Add a new segment to the configuration.
        
        Args:
            start: ROM offset where segment starts
            seg_type: Segment type ('asm', 'c', 'data', etc.)
            name: Optional segment name (also used as file path)
            end: Optional end offset (will add a trailing 'asm' segment)
            use_dict_format: Use dict format (recommended) vs list format
            
        Returns:
            True if segment was added, False if it already exists
        """
        subsegments = self.get_subsegments()
        if subsegments is None:
            raise RuntimeError("No subsegments found in main segment")
        
        # Check if segment already exists at this offset
        if self.segment_exists_at(start):
            return False
        
        # Find insertion point
        insert_idx = self.find_insertion_index(start)
        
        # Create the segment
        if use_dict_format:
            new_seg = self.create_dict_segment(start, seg_type, name)
        else:
            new_seg = self.create_list_segment(start, seg_type, name)
        
        # Insert the segment
        subsegments.insert(insert_idx, new_seg)
        
        # If end is specified, add a trailing asm segment
        if end is not None and not self.segment_exists_at(end):
            end_idx = self.find_insertion_index(end)
            if use_dict_format:
                end_seg = self.create_dict_segment(end, 'asm')
            else:
                end_seg = self.create_list_segment(end, 'asm')
            subsegments.insert(end_idx, end_seg)
        
        return True
    
    def add_range(
        self,
        start_addr: int,
        end_addr: int,
        seg_type: str,
        name: str,
        use_dict_format: bool = True
    ) -> bool:
        """
        Add a segment covering an address range (VRAM addresses).
        
        Args:
            start_addr: VRAM start address (0x80XXXXXX)
            end_addr: VRAM end address (0x80XXXXXX)
            seg_type: Segment type
            name: Segment name (also file path)
            use_dict_format: Use dict format
            
        Returns:
            True if added successfully
        """
        start_rom = vram_to_rom(start_addr)
        end_rom = vram_to_rom(end_addr)
        
        # Check for overlap with C segments
        overlap = self.check_overlap_with_c(start_rom, end_rom)
        if overlap:
            raise ValueError(
                f"Range overlaps with existing C segment '{overlap.name}' "
                f"at 0x{overlap.vram_start:08X}"
            )
        
        return self.add_segment(
            start=start_rom,
            seg_type=seg_type,
            name=name,
            end=end_rom,
            use_dict_format=use_dict_format
        )

    # =========================================================================
    # Validation Methods
    # =========================================================================
    
    def validate(self) -> ValidationResult:
        """
        Validate the splat configuration.
        
        Checks for:
        - Segments in ascending order
        - No duplicate start offsets
        - Valid segment types
        - Function boundaries (for asm segments)
        - Proper section ordering
        
        Returns:
            ValidationResult with errors and warnings
        """
        result = ValidationResult()
        
        subsegments = self.get_subsegments()
        if not subsegments:
            result.add_error("No subsegments found in main segment")
            return result
        
        # Parse all segments
        segments = self.parse_subsegments()
        
        # Check ordering
        self._validate_ordering(segments, result)
        
        # Check for duplicates
        self._validate_no_duplicates(segments, result)
        
        # Check segment types
        self._validate_segment_types(segments, result)
        
        # Check section ordering (rodata before text before data)
        self._validate_section_order(segments, result)
        
        return result
    
    def _validate_ordering(self, segments: List[SegmentInfo], result: ValidationResult) -> None:
        """Check that segments are in ascending order by start offset."""
        prev_start = -1
        for seg in segments:
            if seg.start < prev_start:
                result.add_error(
                    f"Segment out of order: 0x{seg.start:X} comes after 0x{prev_start:X}",
                    segment_index=seg.index,
                    segment_name=seg.name
                )
            prev_start = seg.start
    
    def _validate_no_duplicates(self, segments: List[SegmentInfo], result: ValidationResult) -> None:
        """Check for duplicate start offsets."""
        seen: dict = {}
        for seg in segments:
            if seg.start in seen:
                result.add_error(
                    f"Duplicate start offset 0x{seg.start:X} (also at index {seen[seg.start]})",
                    segment_index=seg.index,
                    segment_name=seg.name
                )
            else:
                seen[seg.start] = seg.index
    
    def _validate_segment_types(self, segments: List[SegmentInfo], result: ValidationResult) -> None:
        """Check that segment types are valid."""
        valid_types = {
            'asm', 'c', 'hasm', 'data', 'rodata', 'sdata', 'sbss', 'bss',
            'bin', 'pad', 'header', 'code', 'end',
            # Section-prefixed types
            '.rodata', '.data', '.sdata', '.sbss', '.bss', '.text'
        }
        
        for seg in segments:
            if seg.seg_type not in valid_types:
                result.add_warning(
                    f"Unknown segment type '{seg.seg_type}'",
                    segment_index=seg.index,
                    segment_name=seg.name
                )
    
    def _validate_section_order(self, segments: List[SegmentInfo], result: ValidationResult) -> None:
        """
        Check that sections follow expected order: rodata, then text/asm, then data.
        
        This is a soft check - violations are warnings, not errors.
        """
        section_order = {'rodata': 0, 'asm': 1, 'c': 1, 'hasm': 1, 'data': 2, 'sdata': 3, 'pad': 4}
        
        last_section_rank = -1
        last_section_type = None
        
        for seg in segments:
            rank = section_order.get(seg.seg_type, 1)  # Default to text section
            
            if rank < last_section_rank:
                result.add_warning(
                    f"Section order: '{seg.seg_type}' at 0x{seg.start:X} comes after '{last_section_type}'",
                    segment_index=seg.index,
                    segment_name=seg.name
                )
            
            last_section_rank = rank
            last_section_type = seg.seg_type
    
    def check_function_boundary(self, vram_addr: int, known_functions: List[int]) -> Tuple[bool, Optional[int]]:
        """
        Check if a VRAM address is at a known function boundary.
        
        Args:
            vram_addr: VRAM address to check
            known_functions: List of known function start addresses
            
        Returns:
            (is_boundary, nearest_function_addr)
        """
        if vram_addr in known_functions:
            return (True, vram_addr)
        
        # Find nearest function
        nearest = None
        nearest_dist = float('inf')
        
        for func_addr in known_functions:
            dist = abs(func_addr - vram_addr)
            if dist < nearest_dist:
                nearest_dist = dist
                nearest = func_addr
        
        return (False, nearest)
    
    def suggest_split_points(self, known_functions: List[int], min_gap: int = 0x1000) -> List[int]:
        """
        Suggest good split points based on function gaps.
        
        Large gaps between functions often indicate compilation unit boundaries,
        which are good places to split segments.
        
        Args:
            known_functions: List of known function start addresses (sorted)
            min_gap: Minimum gap size to consider as a split point
            
        Returns:
            List of suggested split VRAM addresses
        """
        if len(known_functions) < 2:
            return []
        
        sorted_funcs = sorted(known_functions)
        suggestions = []
        
        for i in range(len(sorted_funcs) - 1):
            gap = sorted_funcs[i + 1] - sorted_funcs[i]
            if gap >= min_gap:
                # Suggest the start of the function after the gap
                suggestions.append(sorted_funcs[i + 1])
        
        return suggestions
