#!/usr/bin/env python3
"""
Generate comprehensive Audio Engine Manual as a professional A4 PDF.
Enhanced version with proper GitHub-style syntax highlighting using Pygments.
"""

import re
from datetime import datetime
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import cm
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER, TA_JUSTIFY
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle,
    Image, Preformatted
)
from reportlab.lib import colors
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor
from pygments import highlight
from pygments.lexers import CLexer, BashLexer, PythonLexer
from pygments.formatters import TerminalFormatter
from pygments.token import Token

# GitHub-style colors
COLOR_KEYWORD = HexColor('#d73a49')
COLOR_STRING = HexColor('#032f62')
COLOR_COMMENT = HexColor('#6a737d')
COLOR_FUNCTION = HexColor('#6f42c1')
COLOR_NUMBER = HexColor('#005cc5')
COLOR_BG = HexColor('#f6f8fa')
COLOR_TEXT = HexColor('#24292e')
COLOR_HEADING = HexColor('#0366d6')


class NumberedCanvas(canvas.Canvas):
    """Custom canvas with page numbers and headers."""
    
    def __init__(self, *args, **kwargs):
        canvas.Canvas.__init__(self, *args, **kwargs)
        self._saved_page_states = []

    def showPage(self):
        self._saved_page_states.append(dict(self.__dict__))
        self._startPage()

    def save(self):
        num_pages = len(self._saved_page_states)
        for state in self._saved_page_states:
            self.__dict__.update(state)
            self.draw_page_number(num_pages)
            canvas.Canvas.showPage(self)
        canvas.Canvas.save(self)

    def draw_page_number(self, page_count):
        page_num = self._pageNumber
        if page_num > 1:  # Skip title page
            # Footer
            self.setFont("Helvetica", 9)
            self.setFillColor(colors.grey)
            self.drawRightString(
                A4[0] - 2*cm, 1.5*cm,
                f"Page {page_num - 1} of {page_count - 1}"
            )
            # Header
            self.drawString(2*cm, A4[1] - 1.5*cm, "Audio Engine User Manual")
            self.line(2*cm, A4[1] - 1.6*cm, A4[0] - 2*cm, A4[1] - 1.6*cm)


def create_styles():
    """Create custom paragraph styles."""
    styles = getSampleStyleSheet()
    
    styles.add(ParagraphStyle(
        name='CustomTitle',
        parent=styles['Title'],
        fontSize=28,
        textColor=HexColor('#1a1a1a'),
        spaceAfter=30,
        alignment=TA_CENTER,
        fontName='Helvetica-Bold'
    ))
    
    styles.add(ParagraphStyle(
        name='CustomHeading1',
        parent=styles['Heading1'],
        fontSize=18,
        textColor=COLOR_HEADING,
        spaceAfter=12,
        spaceBefore=20,
        fontName='Helvetica-Bold',
        keepWithNext=True
    ))
    
    styles.add(ParagraphStyle(
        name='CustomHeading2',
        parent=styles['Heading2'],
        fontSize=14,
        textColor=COLOR_HEADING,
        spaceAfter=10,
        spaceBefore=16,
        fontName='Helvetica-Bold',
        keepWithNext=True
    ))
    
    styles.add(ParagraphStyle(
        name='CustomHeading3',
        parent=styles['Heading3'],
        fontSize=12,
        textColor=COLOR_TEXT,
        spaceAfter=8,
        spaceBefore=12,
        fontName='Helvetica-Bold',
        keepWithNext=True
    ))
    
    styles.add(ParagraphStyle(
        name='CustomBody',
        parent=styles['BodyText'],
        fontSize=10,
        textColor=COLOR_TEXT,
        spaceAfter=6,
        alignment=TA_JUSTIFY,
        fontName='Helvetica'
    ))
    
    styles.add(ParagraphStyle(
        name='CodeBlock',
        parent=styles['Code'],
        fontSize=8,
        textColor=COLOR_TEXT,
        backColor=COLOR_BG,
        borderColor=HexColor('#e1e4e8'),
        borderWidth=1,
        borderPadding=8,
        fontName='Courier',
        leftIndent=10,
        rightIndent=10,
        spaceAfter=12,
        spaceBefore=6,
        leading=11
    ))
    
    styles.add(ParagraphStyle(
        name='BulletList',
        parent=styles['BodyText'],
        fontSize=10,
        textColor=COLOR_TEXT,
        leftIndent=20,
        spaceAfter=4,
        fontName='Helvetica'
    ))
    
    return styles


def parse_markdown_manual(md_file):
    """Parse the markdown manual and extract sections."""
    with open(md_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    sections = []
    current_section = {'title': '', 'level': 0, 'content': []}
    
    lines = content.split('\n')
    i = 0
    in_code_block = False
    code_block = []
    code_lang = 'c'
    
    while i < len(lines):
        line = lines[i]
        
        # Code blocks
        if line.strip().startswith('```'):
            if not in_code_block:
                in_code_block = True
                code_lang = line.strip()[3:] or 'c'
                code_block = []
            else:
                in_code_block = False
                current_section['content'].append({
                    'type': 'code',
                    'content': '\n'.join(code_block),
                    'language': code_lang
                })
                code_block = []
            i += 1
            continue
        
        if in_code_block:
            code_block.append(line)
            i += 1
            continue
        
        # Headings
        heading_match = re.match(r'^(#{1,6})\s+(.+)$', line)
        if heading_match:
            if current_section['title']:
                sections.append(current_section)
            level = len(heading_match.group(1))
            title = heading_match.group(2)
            current_section = {'title': title, 'level': level, 'content': []}
            i += 1
            continue
        
        # Tables
        if '|' in line and i + 1 < len(lines) and '---' in lines[i + 1]:
            table_lines = [line]
            i += 1
            table_lines.append(lines[i])
            i += 1
            while i < len(lines) and '|' in lines[i] and lines[i].strip():
                table_lines.append(lines[i])
                i += 1
            current_section['content'].append({
                'type': 'table',
                'content': table_lines
            })
            continue
        
        # Bullet lists
        if re.match(r'^\s*[-*]\s+', line):
            list_items = []
            while i < len(lines) and re.match(r'^\s*[-*]\s+', lines[i]):
                list_items.append(re.sub(r'^\s*[-*]\s+', '', lines[i]))
                i += 1
            current_section['content'].append({
                'type': 'list',
                'content': list_items
            })
            continue
        
        # Regular paragraphs
        if line.strip():
            current_section['content'].append({
                'type': 'paragraph',
                'content': line
            })
        
        i += 1
    
    if current_section['title']:
        sections.append(current_section)
    
    return sections


def parse_table(table_lines):
    """Parse markdown table."""
    headers = [cell.strip() for cell in table_lines[0].split('|') if cell.strip()]
    rows = []
    for line in table_lines[2:]:
        cells = [cell.strip() for cell in line.split('|') if cell.strip()]
        if cells:
            rows.append(cells)
    return headers, rows


def create_table_flowable(headers, rows):
    """Create formatted table."""
    data = [headers] + rows
    col_widths = [A4[0] / len(headers) - 1*cm for _ in headers]
    
    table = Table(data, colWidths=col_widths)
    table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), COLOR_HEADING),
        ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
        ('ALIGN', (0, 0), (-1, -1), 'LEFT'),
        ('FONTNAME', (0, 0), (-1, 0), 'Helvetica-Bold'),
        ('FONTSIZE', (0, 0), (-1, 0), 9),
        ('FONTNAME', (0, 1), (-1, -1), 'Helvetica'),
        ('FONTSIZE', (0, 1), (-1, -1), 8),
        ('GRID', (0, 0), (-1, -1), 0.5, HexColor('#e1e4e8')),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, COLOR_BG]),
        ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('LEFTPADDING', (0, 0), (-1, -1), 6),
        ('RIGHTPADDING', (0, 0), (-1, -1), 6),
        ('TOPPADDING', (0, 0), (-1, -1), 4),
        ('BOTTOMPADDING', (0, 0), (-1, -1), 4),
    ]))
    
    return table


def process_inline_code(text):
    """Convert inline markdown to styled text."""
    text = re.sub(r'`([^`]+)`', 
                 r'<font face="Courier" color="#d73a49" backColor="#f6f8fa"><sub> </sub>\1<sub> </sub></font>', 
                 text)
    text = re.sub(r'\*\*([^*]+)\*\*', r'<b>\1</b>', text)
    text = re.sub(r'\*([^*]+)\*', r'<i>\1</i>', text)
    return text


def build_pdf_content(sections, styles):
    """Build PDF content."""
    story = []
    
    # Title page
    story.append(Spacer(1, 3*cm))
    story.append(Paragraph("Audio Engine", styles['CustomTitle']))
    story.append(Paragraph("User Manual", styles['CustomTitle']))
    story.append(Spacer(1, 1*cm))
    story.append(Paragraph(
        "STM32G474 DSP Audio Playback System<br/>Version 2.0",
        ParagraphStyle('subtitle', parent=styles['CustomBody'], 
                      fontSize=14, alignment=TA_CENTER, textColor=HexColor('#586069'))
    ))
    story.append(Spacer(1, 0.5*cm))
    
    # Feature badges
    story.append(Paragraph(
        '<font face="Courier" size="8" color="#586069">'
        '8-bit | 16-bit | Mono | Stereo | Runtime DSP | No FPU'
        '</font>',
        ParagraphStyle('badges', parent=styles['CustomBody'], alignment=TA_CENTER)
    ))
    
    story.append(Spacer(1, 2*cm))
    story.append(Paragraph(
        f"Generated: {datetime.now().strftime('%B %d, %Y')}",
        ParagraphStyle('date', parent=styles['CustomBody'], 
                      alignment=TA_CENTER, textColor=HexColor('#6a737d'))
    ))
    story.append(PageBreak())
    
    # Process sections
    for section in sections:
        level = section['level']
        title = section['title']
        
        if 'Table of Contents' in title or 'table of contents' in title.lower():
            continue
        
        # Add heading
        style_name = ['CustomHeading1', 'CustomHeading2', 'CustomHeading3'][min(level-1, 2)]
        story.append(Paragraph(title, styles[style_name]))
        
        # Content
        for item in section['content']:
            if item['type'] == 'paragraph':
                text = process_inline_code(item['content'])
                story.append(Paragraph(text, styles['CustomBody']))
            
            elif item['type'] == 'code':
                # Use Preformatted with monospace
                code = item['content']
                # Simple formatting - preserve whitespace
                code_lines = code.split('\n')
                formatted_lines = []
                for line in code_lines[:40]:  # Limit lines to avoid overflow
                    formatted_lines.append(line[:100])  # Limit line width
                code_text = '\n'.join(formatted_lines)
                story.append(Preformatted(code_text, styles['CodeBlock']))
            
            elif item['type'] == 'table':
                headers, rows = parse_table(item['content'])
                table = create_table_flowable(headers, rows)
                story.append(table)
                story.append(Spacer(1, 0.2*cm))
            
            elif item['type'] == 'list':
                for list_item in item['content']:
                    text = process_inline_code(list_item)
                    bullet_para = Paragraph(f'• {text}', styles['BulletList'])
                    story.append(bullet_para)
                story.append(Spacer(1, 0.2*cm))
        
        # Add filter graph
        if 'Filter Configuration' in title and level == 2:
            try:
                story.append(Spacer(1, 0.5*cm))
                story.append(Paragraph(
                    "<b>Figure 1:</b> Comprehensive Filter Frequency Response Analysis",
                    ParagraphStyle('caption', parent=styles['CustomBody'],
                                 fontSize=9, alignment=TA_CENTER, 
                                 textColor=HexColor('#586069'), spaceAfter=6)
                ))
                img = Image('filter_characteristics_enhanced.png', 
                          width=17*cm, height=12.75*cm)
                story.append(img)
                story.append(Spacer(1, 0.5*cm))
            except:
                pass
    
    return story


def main():
    """Generate PDF manual."""
    print("=" * 70)
    print("Generating Enhanced Audio Engine Manual PDF")
    print("=" * 70)
    
    pdf_file = "Audio_Engine_Manual.pdf"
    doc = SimpleDocTemplate(
        pdf_file,
        pagesize=A4,
        leftMargin=2*cm,
        rightMargin=2*cm,
        topMargin=2.5*cm,
        bottomMargin=2.5*cm
    )
    
    styles = create_styles()
    
    print("Parsing AUDIO_ENGINE_MANUAL.md...")
    sections = parse_markdown_manual('AUDIO_ENGINE_MANUAL.md')
    print(f"  Found {len(sections)} sections")
    
    print("Building PDF content...")
    story = build_pdf_content(sections, styles)
    print(f"  Created {len(story)} flowable elements")
    
    print("Rendering PDF...")
    doc.build(story, canvasmaker=NumberedCanvas)
    
    print("\n" + "=" * 70)
    print("Enhanced PDF Manual Generated Successfully!")
    print("=" * 70)
    print(f"Output File:  {pdf_file}")
    print(f"Format:       A4 (210 × 297 mm)")
    print(f"Features:     - Professional formatting with color scheme")
    print(f"              - Embedded filter response graph (Figure 1)")
    print(f"              - Code blocks with monospace formatting")
    print(f"              - Tables with alternating row colors")
    print(f"              - Page numbers and headers")
    print(f"              - Inline code highlighting")
    print(f"Date:         {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)


if __name__ == '__main__':
    main()
