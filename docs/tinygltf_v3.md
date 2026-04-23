# tinygltf3 Reference Guide

This document explains how to work with tinygltf3 in this repository:

- Overall parse and traversal workflow
- Core glTF data concepts (accessor, buffer view, buffer)
- How to access each major category of loaded model data
	- Vertices and attributes
	- Indices
	- Materials, textures, samplers, images
	- Scenes and nodes
	- Animations and skins
	- Extensions and extras

The source of truth for API/struct names is `ThirdParty/tiny_gltf_v3.h`.

## 1. Quick Mental Model

glTF stores geometry data with three layers:

- `buffer`: raw bytes
- `bufferView`: a byte range inside a buffer
- `accessor`: typed interpretation of elements inside a bufferView

Think of it as:

`bytes -> slice -> typed array`

Example:

- A POSITION attribute is usually an accessor with:
	- `component_type = FLOAT`
	- `type = VEC3`
	- `count = number of vertices`

## 2. Lifetime and Ownership

`tg3_model` uses arena-backed storage. After parse succeeds, all pointers in the model point into memory owned by the model.

Important rules:

- Always call `tg3_model_free(&model)` when done.
- Never `free()` model sub-pointers yourself.
- Any pointer into model memory becomes invalid after `tg3_model_free`.
- If you need persistent runtime data, copy it out first.

## 3. Parse Workflow

Minimal parse flow:

```c
tg3_model model = {0};
tg3_error_stack errors;
tg3_parse_options opts;

tg3_error_stack_init(&errors);
tg3_parse_options_init(&opts);

// Optional tuning
opts.strictness = TG3_PERMISSIVE; // or TG3_STRICT
opts.parse_float32 = 1;           // faster JSON float parse, less precision

tg3_error_code rc = tg3_parse_file(
		&model,
		&errors,
		path,
		(uint32_t)strlen(path),
		&opts
);

if (rc != TG3_OK) {
		uint32_t n = tg3_errors_count(&errors);
		for (uint32_t i = 0; i < n; ++i) {
				const tg3_error_entry *e = tg3_errors_get(&errors, i);
				// log e->message, e->json_path, e->code
		}
		tg3_error_stack_free(&errors);
		return;
}

// ... use model ...

tg3_model_free(&model);
tg3_error_stack_free(&errors);
```

Parse entry points:

- `tg3_parse` -> JSON from memory
- `tg3_parse_glb` -> GLB from memory
- `tg3_parse_auto` -> auto-detect JSON vs GLB from memory
- `tg3_parse_file` -> file path input

## 4. Data Layout in tg3_model

`tg3_model` is mostly flat arrays + counts, for example:

- `meshes` / `meshes_count`
- `accessors` / `accessors_count`
- `buffers` / `buffers_count`
- `buffer_views` / `buffer_views_count`
- `materials`, `textures`, `images`, `samplers`
- `nodes`, `scenes`, `animations`, `skins`

Indices across structs are 0-based. `-1` (`TG3_INDEX_NONE`) means "not present".

Always bounds-check before dereferencing an index.

## 5. Accessor, BufferView, Buffer Explained

### Accessor

An accessor tells you:

- Which bufferView to read (`buffer_view`)
- Byte offset within that view (`byte_offset`)
- Element count (`count`)
- Element type (`type`, like SCALAR, VEC2, VEC3, MAT4)
- Component type (`component_type`, like FLOAT, UNSIGNED_SHORT)

### BufferView

A bufferView tells you:

- Which buffer (`buffer`)
- Range start (`byte_offset`) and length (`byte_length`)
- Optional stride (`byte_stride`)

### Buffer

A buffer contains raw bytes in `data.data` with size `data.count`.

### Byte address formula

For element `i`:

```text
address = buffer.data
				+ bufferView.byte_offset
				+ accessor.byte_offset
				+ i * stride
```

Where:

- `stride = bufferView.byte_stride` when non-zero
- Otherwise `stride = component_size * num_components`

Helpers:

- `tg3_component_size(accessor.component_type)`
- `tg3_num_components(accessor.type)`
- `tg3_accessor_byte_stride(&accessor, &bufferView)`

## 6. Accessing Vertex Attributes

Path:

`model.meshes[m] -> primitive -> primitive.attributes -> accessor`

Each primitive attribute is a key-value pair:

- key: semantic string (`"POSITION"`, `"NORMAL"`, `"TEXCOORD_0"`, ...)
- value: accessor index

Use `tg3_str_equals_cstr(attr.key, "POSITION")` for comparisons.

Example helper:

```c
static int32_t find_attr_accessor(const tg3_primitive *prim, const char *semantic) {
		for (uint32_t i = 0; i < prim->attributes_count; ++i) {
				if (tg3_str_equals_cstr(prim->attributes[i].key, semantic)) {
						return prim->attributes[i].value;
				}
		}
		return TG3_INDEX_NONE;
}
```

## 7. Accessing Indices

Index buffer is `primitive.indices` (accessor index).

Expected index accessor constraints:

- `type == TG3_TYPE_SCALAR`
- `component_type` in:
	- `TG3_COMPONENT_TYPE_UNSIGNED_BYTE`
	- `TG3_COMPONENT_TYPE_UNSIGNED_SHORT`
	- `TG3_COMPONENT_TYPE_UNSIGNED_INT`

Decode each index based on component type and widen to `uint32_t` in your engine-side index array.

If `primitive.indices == TG3_INDEX_NONE`, treat as non-indexed draw with implicit sequence `0..count-1`.

## 8. Accessing Materials, Textures, and Images

Typical chain for base color texture:

`primitive.material -> material.pbr_metallic_roughness.base_color_texture.index -> texture.source -> image`

Fields to use:

- `tg3_material`
	- `pbr_metallic_roughness`
	- `normal_texture`, `occlusion_texture`, `emissive_texture`
	- `alpha_mode`, `alpha_cutoff`, `double_sided`
- `tg3_texture`
	- `source` (image index)
	- `sampler` (sampler index)
- `tg3_image`
	- decoded pixel payload in `image.data` with byte count `image.count`
	- `width`, `height`, `component`, `bits`, `mime_type`
- `tg3_sampler`
	- filtering and wrap modes

Notes:

- `-1` means field absent.
- `opts.images_as_is = 1` changes image handling behavior (raw encoded payload path).

## 9. Accessing Scene Graph Data

Top-level render roots come from scene nodes:

- choose scene: `model.default_scene` or explicit scene index
- scene has `nodes[]` root indices
- each node has transform + optional mesh + children

Node transform rules:

- If `has_matrix == 1`, use `matrix` directly
- Otherwise compose from TRS:
	- `translation`
	- `rotation` quaternion `[x,y,z,w]`
	- `scale`

Node references:

- `node.mesh` -> mesh index
- `node.camera` -> camera index
- `node.skin` -> skin index
- `node.light` -> light index (extension-backed)

## 10. Accessing Animation Data

Animation model:

- `tg3_animation`
	- `channels[]`
	- `samplers[]`

Channel points to target node property:

- `target.node`
- `target.path` in `"translation"`, `"rotation"`, `"scale"`, `"weights"`

Sampler points to accessors:

- `input` accessor -> keyframe time values
- `output` accessor -> keyframe data
- `interpolation` -> `LINEAR`, `STEP`, `CUBICSPLINE`

## 11. Accessing Skinning Data

`tg3_skin` contains:

- `joints[]` node indices
- `inverse_bind_matrices` accessor index (optional)
- `skeleton` root node index (optional)

The inverse bind matrices accessor typically reads MAT4 float data.

## 12. Accessing Extras and Extensions

Most structs include `ext` (`tg3_extras_ext`) with:

- `extras` generic value tree (`tg3_value`)
- `extensions[]` list of named extension payloads

Use this for custom engine metadata and extension-specific logic.

If `opts.store_original_json = 1`, raw JSON text for extras/extensions is preserved in `extras_json` and `extensions_json`.

## 13. Common Pitfalls

- Using model pointers after `tg3_model_free`.
- Skipping index bounds checks on cross-references.
- Assuming all primitives are triangles.
	- Check `primitive.mode`; default is triangles when mode is unset.
- Assuming all attributes exist.
	- Many assets omit NORMAL/TANGENT/TEXCOORD_1.
- Assuming tightly packed buffer data.
	- Respect `byte_stride` when present.
- Ignoring `normalized` flag on integer accessors.
- Forgetting that morph targets are per-primitive and optional.

## 14. Recommended Engine Integration Pattern

For this repository's loading path:

1. Parse glTF into `tg3_model`.
2. Traverse meshes/primitives and decode attributes + indices into local CPU vectors.
3. Copy decoded runtime data to your own storage (for example, `ObjectStorage`).
4. Extract texture/material metadata needed by your renderer.
5. Free model (`tg3_model_free`) once copy-out is complete.

This avoids lifetime bugs and keeps renderer ownership explicit.

## 15. Minimal Geometry Extraction Skeleton

```c
for (uint32_t m = 0; m < model.meshes_count; ++m) {
		const tg3_mesh *mesh = &model.meshes[m];
		for (uint32_t p = 0; p < mesh->primitives_count; ++p) {
				const tg3_primitive *prim = &mesh->primitives[p];

				int32_t pos_acc_idx = find_attr_accessor(prim, "POSITION");
				if (pos_acc_idx < 0) continue;

				const tg3_accessor *pos_acc = &model.accessors[pos_acc_idx];
				if (pos_acc->buffer_view < 0 || pos_acc->buffer_view >= (int32_t)model.buffer_views_count) continue;

				const tg3_buffer_view *pos_bv = &model.buffer_views[pos_acc->buffer_view];
				if (pos_bv->buffer < 0 || pos_bv->buffer >= (int32_t)model.buffers_count) continue;

				const tg3_buffer *pos_buf = &model.buffers[pos_bv->buffer];
				int32_t stride = tg3_accessor_byte_stride(pos_acc, pos_bv);
				if (stride <= 0) continue;

				const uint8_t *base = pos_buf->data.data + pos_bv->byte_offset + pos_acc->byte_offset;
				for (uint64_t i = 0; i < pos_acc->count; ++i) {
						const float *xyz = (const float *)(base + i * (uint64_t)stride);
						// xyz[0], xyz[1], xyz[2]
				}

				if (prim->indices >= 0) {
						const tg3_accessor *idx_acc = &model.accessors[prim->indices];
						// decode UNSIGNED_BYTE / UNSIGNED_SHORT / UNSIGNED_INT
				}
		}
}
```

---

If you add or rename tinygltf3 fields/macros in `tiny_gltf_v3.h`, update this page immediately so it stays the local source of truth for the team.
