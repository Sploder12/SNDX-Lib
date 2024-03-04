#pragma once

#include "atlas.hpp"

#include "../util/atlasbuild.hpp"

#include <unordered_map>
#include <optional>

#include <ft2build.h>
#include <freetype/freetype.h>

#include <gl/glew.h>

namespace sndx {

	struct FreetypeContext {
		FT_Library ft = nullptr;

		FreetypeContext() {
			FT_Init_FreeType(&ft);
		}

		operator FT_Library&() {
			return ft;
		}

		FreetypeContext(const FreetypeContext&) = delete;
		FreetypeContext(FreetypeContext&& other) noexcept:
			ft(std::move(other.ft)) {
			other.ft = nullptr;
		}

		FreetypeContext& operator=(const FreetypeContext&) = delete;
		FreetypeContext& operator=(FreetypeContext&& other) noexcept {
			std::swap(ft, other.ft);
			return *this;
		}

		~FreetypeContext() {
			FT_Done_FreeType(ft);
			ft = nullptr;
		}
	};

	// metrics a character
	struct gmetric {
		float advance;
		glm::vec2 bearing;
		glm::vec2 dims;

		constexpr gmetric() :
			advance(0), bearing(0.0f), dims(0.0f) {}

		constexpr gmetric(float advance, int bearingX, int bearingY, unsigned short width, unsigned short height) :
			advance(advance), bearing(bearingX, bearingY), dims(width, height) {}
	};

	struct Font {
		// metrics for all the characters
		std::unordered_map<FT_ULong, gmetric> metrics;
		Atlas<FT_ULong> atlas;

		int maxBearingY = 0; // Needed for hang calculation
		bool sdf = false;

		// returns charcode if it exists, the charcode "white square" otherwise
		[[nodiscard]]
		FT_ULong getChar(FT_ULong charcode) const {
			if (!contains(charcode)) return 9633;

			return charcode;
		}

		[[nodiscard]]
		bool contains(FT_ULong charcode) const {
			return metrics.contains(charcode);
		}

		[[nodiscard]]
		const gmetric& getCharMetrics(FT_ULong charcode) const {
			return metrics.at(getChar(charcode));
		}

		[[nodiscard]]
		glm::vec2 getCharOffset(FT_ULong charcode) const {
			return atlas.getCell(getChar(charcode)).first;
		}

		[[nodiscard]]
		glm::vec2 getCharDims(FT_ULong charcode) const {
			return atlas.getCell(getChar(charcode)).second;
		}

		void bind(size_t id = 0) const {
			atlas.bind(id);
		}

		void destroy() {
			metrics.clear();
			atlas.destroy();
		}
	};


	template <size_t font_padding = 2> [[nodiscard]]
	inline std::optional<Font> loadFont(FreetypeContext& context, const char* filepath, bool sdf = false, unsigned int size = 32) {
		FT_Face face;
		if (FT_New_Face(context, filepath, 0, &face)) return {};

		// Set the pixel height to the size, the width is derived
		FT_Set_Pixel_Sizes(face, 0, size);

		//if (sdf) size *= 2;

		FT_ULong count = face->num_glyphs;
		unsigned int columns = (unsigned int)(ceil(sqrt(count)));

		Font out;
		out.metrics.reserve(count);
		out.sdf = sdf;

		AtlasBuilder<FT_ULong> builder{};

		// Iterate over each character
		FT_UInt idx;
		FT_ULong chr = FT_Get_First_Char(face, &idx);
		while (idx != 0) {
			FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);

			const FT_GlyphSlot& glyph = face->glyph;

			// Have freetype render the glyph
			if (FT_Render_Glyph(glyph, (sdf) ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL)) {
				FT_Done_Face(face);
				return {};
			}

			const FT_Bitmap& bitmap = glyph->bitmap;

			ImageData img;
			img.width = bitmap.width;
			img.height = bitmap.rows;
			img.channels = 1;
			img.data.resize((long long)(img.width) * img.height * img.channels, 0);

			std::copy(bitmap.buffer, bitmap.buffer + img.data.size(), img.data.begin());

			builder.insert(chr, std::move(img));

			// Needed for proper text y alignment on the baseline
			if (glyph->bitmap_top > out.maxBearingY) out.maxBearingY = glyph->bitmap_top;

			// create the metric for the font, the advance is in 64ths of pixels so divide by 64 to get it into pixels
			out.metrics[chr] = gmetric(glyph->advance.x / 64.0f, glyph->bitmap_left, glyph->bitmap_top, glyph->bitmap.width, glyph->bitmap.rows);
		
			chr = FT_Get_Next_Char(face, chr, &idx);
		}
		FT_Done_Face(face);
	
		out.atlas = builder.buildAtlas<font_padding>(columns / 4.0f);
	
		return out;
	}

	using CharPosWH = std::pair<glm::vec2, glm::vec2>;

	struct TextLocationResult {
		// the position is the top-left of the glyph
		std::vector<CharPosWH> result{};

		// the cursor after the last character
		glm::vec2 endPos;
	};

	template <typename C = char>
	inline TextLocationResult getTextLocations(const Font& font, std::basic_string_view<C> text, glm::vec2 pos, glm::vec2 scale, bool center) {
		pos.y -= font.maxBearingY * scale.y;
		
		if (center) {
			for (const auto& chr : text) {
				const auto& metrics = font.getCharMetrics(FT_ULong(chr));
				pos.x -= metrics.advance * scale.x * 0.5f;
			}
		}

		TextLocationResult out{};
		out.result.reserve(text.size());

		for (const auto& chr : text) {
			const auto& metrics = font.getCharMetrics(FT_ULong(chr));

			auto charPos = pos + metrics.bearing * scale;
			auto charDims = metrics.dims * scale;

			out.result.emplace_back(charPos, charDims);

			pos.x += metrics.advance * scale.x;
		}

		out.endPos = pos;
		return out;
	}
}