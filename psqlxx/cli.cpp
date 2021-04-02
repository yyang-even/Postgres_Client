#include <psqlxx/cli.hpp>

#include <cassert>

#include <filesystem>
#include <iostream>

#include <histedit.h>


namespace fs = std::filesystem;

namespace {

[[nodiscard]]
inline const char *prompt(const std::string &/*user_name = "postgres"*/) {
    // return (user_name + " > ").c_str();
    return "postgres > ";
}

[[nodiscard]]
inline const std::string getDefaultHistoryFile() {
    fs::path home_dir(getenv("HOME"));
    return (home_dir / ".postgres-client.hist").string();
}

}


namespace psqlxx {

CliOptions::CliOptions(): history_file(getDefaultHistoryFile()) {
}


Cli::Cli(const char *program_path, CliOptions options) : m_options(std::move(options)),
    m_el(el_init(program_path, stdin, stdout, stderr)),
    m_history(history_init()),
    m_ev(new HistEvent()),
    m_tokenizer(tok_init(nullptr)) {
}

Cli::~Cli() {
    tok_end(m_tokenizer);

    history(m_history, m_ev.get(), H_SAVE, m_options.history_file.c_str());
    history_end(m_history);

    el_end(m_el);
}

void Cli::Config() const {
    history(m_history, m_ev.get(), H_SETSIZE, m_options.history_size);
    history(m_history, m_ev.get(), H_SETUNIQUE, 1);

    el_set(m_el, EL_HIST, history, m_history);

    el_set(m_el, EL_EDITOR, m_options.editor.c_str());

    el_set(m_el, EL_SIGNAL, 1);

    el_set(m_el, EL_PROMPT, prompt);

    // el_set(m_el, EL_ADDFN, "ed-complete", "Complete argument", complete);

    // el_set(m_el, EL_BIND, "^I", "ed-complete", NULL);
    el_set(m_el, EL_BIND, "\b", "ed-delete-next-char", NULL);
    el_set(m_el, EL_BIND, "\033[A", "ed-prev-history", NULL);
    el_set(m_el, EL_BIND, "^r", "em-inc-search-prev", NULL);
    el_set(m_el, EL_BIND, "-a", "k", "ed-prev-line", NULL);
    el_set(m_el, EL_BIND, "-a", "j", "ed-next-line", NULL);

    el_source(m_el, m_options.editrc_file);
    history(m_history, m_ev.get(), H_LOAD, m_options.history_file.c_str());
}

void Cli::Run() const {
    const char *a_line = nullptr;
    int line_length = 0;
    bool previous_line_completed = true;

    while ((a_line = el_gets(m_el, &line_length)) and line_length != 0)  {
        // Ignore empty lines
        if (previous_line_completed && line_length == 1) {
            continue;
        }

        int word_count = 0;
        const char **words = nullptr;
        const auto tokenize_result = tok_str(m_tokenizer, a_line, &word_count, &words);
        if (tokenize_result < 0) {
            std::cerr << "Internal error." << std::endl;
            continue;
        }
        const auto current_line_completed = not(tokenize_result > 0);

        history(m_history, m_ev.get(), previous_line_completed ? H_ENTER : H_APPEND, a_line);
        previous_line_completed = current_line_completed;
        if (not current_line_completed) {
            continue;
        }

        if (word_count <= 0) {
            continue;
        }

        m_line_handler(a_line);

        tok_reset(m_tokenizer);
    }
}

void Cli::RegisterLineHandler(const LineHandlerType handler) {
    m_line_handler = handler;
}

}//namespace psqlxx
