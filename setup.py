from setuptools import Extension, setup

setup(
    name="wordle_solver",
    version="1.0.0",
    ext_modules= [
        Extension(
            name="wordle_solver",
            sources=["src/wordle.c", "src/wordle_wrapper.c"],
            include_dirs = ["include"],
        )
    ],
    install_requires=["pygame"]
)