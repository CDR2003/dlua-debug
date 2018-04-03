#include <debugger/impl.h>
#include <debugger/io/helper.h>
#include <debugger/io/base.h>
#include <rapidjson/schema.h>
#include <rapidjson/error/en.h>
#include <fstream>

#if 1
#	define log(...)
#else

#include <base/util/format.h>
#include <Windows.h>

template <class... Args>
static void log(const char* fmt, const Args& ... args)
{
	auto s = base::format(fmt, args...);
	OutputDebugStringA(s.c_str());
}

#endif	 

namespace vscode {

	class file {
	public:
		file(const wchar_t* filename, std::ios_base::openmode mode) {
			file_.open(filename, std::ios::binary | mode);
		}
		~file() {
			file_.close();
		}
		bool is_open() const {
			return !!file_;
		}
		template <class SequenceT>
		SequenceT read() {
			return std::move(SequenceT((std::istreambuf_iterator<char>(file_)), (std::istreambuf_iterator<char>())));
		}
		template <class SequenceT>
		void write(SequenceT buf) {
			std::copy(buf.begin(), buf.end(), std::ostreambuf_iterator<char>(file_));
		}
	private:
		file();
		std::fstream file_;
	};

	schema* io_schema(const std::wstring& schemafile)
	{
		file file(schemafile.c_str(), std::ios_base::in);
		if (!file.is_open()) {
			return nullptr;
		}
		std::string buf = file.read<std::string>();
		rapidjson::Document sd;
		if (sd.Parse(buf.data(), buf.size()).HasParseError()) {
			log("Input is not a valid JSON\n");
			log("Error(offset %u): %s\n", static_cast<unsigned>(sd.GetErrorOffset()), rapidjson::GetParseError_En(sd.GetParseError()));
			return nullptr;
		}
		return new schema(sd);
	}

	rprotocol io_input(io::base* io, schema* schema)
	{
		std::string buf;
		if (!io->input(buf)) {
			return rprotocol();
		}
		rapidjson::Document	d;
		if (d.Parse(buf.data(), buf.size()).HasParseError())
		{
			log("Input is not a valid JSON\n");
			log("Error(offset %u): %s\n", static_cast<unsigned>(d.GetErrorOffset()), rapidjson::GetParseError_En(d.GetParseError()));
			io->close();
			return rprotocol();
		}
		if (schema)
		{
			rapidjson::SchemaValidator validator(*schema);
			if (!d.Accept(validator))
			{
				rapidjson::StringBuffer sb;
				validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
				log("Invalid schema: %s\n", sb.GetString());
				log("Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
				sb.Clear();
				validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
				log("Invalid document: %s\n", sb.GetString());		
				io->close();
				return rprotocol();
			}
		}
		log("%s\n", buf.data());
		return rprotocol(std::move(d));
	}

	void io_output(io::base* io, const wprotocol& wp)
	{
		if (!wp.IsComplete())
			return;
		if (!io->output(wp.data(), wp.size()))
			return;
		log("%s\n", wp.data());
	}
}
